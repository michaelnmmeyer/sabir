#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>

#include "lib/utf8proc.h"
#include "api.h"

#ifdef SB_DEBUG
static const bool sb_debug = true;
#else
static const bool sb_debug = false;
#endif
bool sb_verbose;

#define SB_NGRAM_SIZE 4
#define SB_PAD_CHAR 0xff

struct sabir {
   size_t num_labels;
   size_t table_mask;
   const char *const *labels;
   double *probs;
   uint8_t buf[SB_NGRAM_SIZE];      /* Current quadgram (rolling buffer). */
   size_t buf_pos;                  /* Current write pos in this buffer. */
   uint8_t pending[SB_NGRAM_SIZE];
   ssize_t pending_have;
   double model[];
};

const char *sb_strerror(int err)
{
   static const char *const tbl[] = {
      [SB_OK] = "no error",
      [SB_EOPEN] = "cannot open model",
      [SB_EMAGIC] = "not a model file",
      [SB_EMODEL] = "invalid model file",
      [SB_EIO] = "I/O error",
      [SB_ENOMEM] = "out of memory",
   };

   if (err >= 0 && (size_t)err < sizeof tbl / sizeof *tbl)
      return tbl[err];
   return "unknown error";
}

void sb_init(struct sabir *sb)
{
   sb->buf[0] = SB_PAD_CHAR;
   sb->buf_pos = 1;
   for (size_t i = 0; i < sb->num_labels; i++)
      sb->probs[i] = 0.;
   sb->pending_have = 0;
}

static size_t sb_pad(size_t n, size_t align)
{
   return n + ((n + align - 1) & ~(align - 1));
}

static bool sb_is_pow2(size_t n)
{
   return n && (n & (n - 1)) == 0;
}

#define SB_MAX_LABELS 255
#define SB_MAX_LABELS_LEN 2048
#define SB_MAX_FEATURES 400000   /* Prevent overflow issues. */

int sb_load(struct sabir **sbp, const char *path)
{
   *sbp = NULL;
   struct sabir *sb = NULL;

   FILE *fp = fopen(path, "r");
   if (!fp)
      return SB_EOPEN;

   /* Magic identifier and version. */
   if (fscanf(fp, "@ sabir 1\n") == EOF) {
      fclose(fp);
      return SB_EMAGIC;
   }

   /* Sections size. */
   size_t num_labels, labels_len, num_features;
   if (fscanf(fp, "> %zu %zu %zu\n", &num_labels, &labels_len, &num_features) != 3)
      goto bad_model;
   if (num_labels == 0 || num_labels > SB_MAX_LABELS)
      goto bad_model;
   if (labels_len == 0 || labels_len > SB_MAX_LABELS_LEN)
      goto bad_model;
   if (num_features == 0 || num_features > SB_MAX_FEATURES || !sb_is_pow2(num_features))
      goto bad_model;

   /* Compute memory offsets. */
   size_t ptrs_off = sb_pad(offsetof(struct sabir, model) + num_features * sizeof(*sb->model), alignof(char *));
   size_t strs_off = sb_pad(ptrs_off + (num_labels + 1) * sizeof(char **), alignof(char));
   size_t probs_off = sb_pad(strs_off + labels_len + num_labels + 1, alignof(double));
   size_t total = probs_off + num_labels * sizeof(double);

   /* Initialize our struct. */
   sb = malloc(total);
   if (!sb) {
      fclose(fp);
      return SB_ENOMEM;
   }
   sb->num_labels = num_labels;
   sb->table_mask = num_features - 1;
   sb->labels = (void *)((char *)sb + ptrs_off);
   sb->probs = (void *)((char *)sb + probs_off);
   sb_init(sb);

   char **ptrs = (void *)((char *)sb + ptrs_off);
   char *strs = (void *)((char *)sb + strs_off);

   /* Read labels. One per line. */
   size_t pos = 0;
   for (size_t i = 0; i < num_labels; i++) {
      char *line = fgets(&strs[pos], labels_len + num_labels + 1 - pos, fp);
      if (!line)
         goto bad_model;
      size_t len = strlen(line);
      if (len < 2 || line[len - 1] != '\n')
         goto bad_model;
      line[len - 1] = '\0';
      ptrs[i] = &strs[pos];
      pos += len;
   }
   ptrs[num_labels] = NULL;
   if (pos != labels_len + num_labels)
      goto bad_model;

   /* Read features. One per line. */
   for (size_t i = 0; i < num_features; i++) {
      uint64_t n;
      if (fscanf(fp, "%"SCNu64"\n", &n) != 1 || n > DBL_MAX - 1)
         goto bad_model;
      sb->model[i] = log(n + 1);
   }

   /* Should have reached the end of the file by now. */
   if (getc(fp) != EOF)
      goto bad_model;

   *sbp = sb;
   fclose(fp);
   return SB_OK;

bad_model: {
   int err = ferror(fp);
   fclose(fp);
   free(sb);
   return err ? SB_EIO : SB_EMODEL;
}
}

void sb_dealloc(struct sabir *sb)
{
   free(sb);
}

const char *const *sb_langs(struct sabir *sb, size_t *nr)
{
   if (nr)
      *nr = sb->num_labels;
   return sb->labels;
}

static uint32_t sb_hash_feature(const uint8_t s[static SB_NGRAM_SIZE], size_t pos)
{
   uint32_t h = 1315423911;
   for (size_t i = 0; i < SB_NGRAM_SIZE; i++)
      h ^= (h << 5) + s[(pos + i) % SB_NGRAM_SIZE] + (h >> 2);
   return h;
}

static uint32_t sb_hash_lang(uint32_t h, uint32_t lang)
{
   h ^= (h << 5) + lang + (h >> 2);
   return h;
}

static bool sb_is_letter(int32_t c)
{
   const utf8proc_property_t *p = utf8proc_get_property(c);

   switch (p->category) {
   case UTF8PROC_CATEGORY_LU:
   case UTF8PROC_CATEGORY_LL:
   case UTF8PROC_CATEGORY_LT:
   case UTF8PROC_CATEGORY_LM:
   case UTF8PROC_CATEGORY_LO:
      return true;
   default:
      return false;
   }
}

static void sb_report(const uint8_t *gram, size_t pos, const char *lang,
                      uint32_t hash, double prob)
{
   for (size_t i = 0; i < SB_NGRAM_SIZE; i++) {
      int c = gram[(pos + i) % SB_NGRAM_SIZE];
      printf("%02x", c);
   }
   printf(" %s %"PRIu32" %la\n", lang, hash, prob);
}

static void sb_update_probs(struct sabir *sb,
                            const uint8_t gram[static SB_NGRAM_SIZE],
                            size_t pos)
{
   uint32_t h1 = sb_hash_feature(gram, pos);

   for (size_t i = 0; i < sb->num_labels; i++) {
      uint32_t h2 = sb_hash_lang(h1, i);
      double prob = sb->model[h2 & sb->table_mask];
      sb->probs[i] += prob;
      if (sb_debug && sb_verbose)
         sb_report(gram, pos, sb->labels[i], h2, prob);
   }
}

static void sb_put_byte(struct sabir *sb, int c)
{
   sb->buf[sb->buf_pos++ % SB_NGRAM_SIZE] = c;
   if (sb->buf_pos >= SB_NGRAM_SIZE)
      sb_update_probs(sb, sb->buf, sb->buf_pos);
}

static ssize_t sb_complete(struct sabir *sb, const uint8_t *text, ssize_t len)
{
   ssize_t clen = utf8proc_utf8class[*sb->pending];
   const ssize_t need = clen - sb->pending_have;

   if (need > len) {
      for (ssize_t j = 0; j < len; j++)
         sb->pending[sb->pending_have++] = text[j];
      return len;
   }

   for (ssize_t j = 0; j < need; j++)
      sb->pending[sb->pending_have++] = text[j];
   sb->pending_have = 0;

   int32_t c;
   clen = utf8proc_iterate(sb->pending, clen, &c);
   if (clen <= 0)
      return 0;
   
   if (sb_is_letter(c)) {
      for (ssize_t j = 0; j < clen; j++)
         sb_put_byte(sb, sb->pending[j]);
   } else {
      sb_put_byte(sb, SB_PAD_CHAR);
      sb->buf[0] = SB_PAD_CHAR;
      sb->buf_pos = 1;      
   }
   return need;
}

static void sb_process(struct sabir *sb, const uint8_t *text, ssize_t len)
{
   /* Complete the last truncated UTF-8 sequence if applicable. */
   ssize_t i = sb->pending_have ? sb_complete(sb, text, len) : 0;
   ssize_t clen;

   for ( ; i < len; i += clen) {
      int32_t c;
      clen = utf8proc_iterate(&text[i], len - i, &c);
      if (clen <= 0) {
         /* The last UTF-8 sequence of a chunk might be truncated. We cannot
          * feed byte ngrams to the classifier until we know the category of
          * the code point to be decoded, so save what we have now and see later
          * if we can complete the sequence.
          */
         clen = utf8proc_utf8class[text[i]];
         if (i + clen > len) {
            sb->pending_have = len - i;
            for (ssize_t j = 0; j < sb->pending_have; j++)
               sb->pending[j] = text[i + j];
            break;
         }
         clen = 1;
         continue;
      }
      if (sb_is_letter(c)) {
         for (ssize_t j = 0; j < clen; j++)
            sb_put_byte(sb, text[i + j]);
      } else {
         sb_put_byte(sb, SB_PAD_CHAR);
         sb->buf[0] = SB_PAD_CHAR;
         sb->buf_pos = 1;
      }
   }
}

#ifndef SSIZE_MAX
   #define SSIZE_MAX (SIZE_MAX / 2)
#endif

void sb_feed(struct sabir *sb, const void *chunk, size_t len)
{
   sb_process(sb, chunk, len < SSIZE_MAX ? len : SSIZE_MAX);
}

const char *sb_finish(struct sabir *sb)
{
   /* Handle the last ngram. */
   sb_put_byte(sb, SB_PAD_CHAR);

   /* Just in case the caller attempts to call this function several times. */
   sb->buf_pos = 0;

   size_t best = 0;
   for (size_t i = 0; i < sb->num_labels; i++)
      if (sb->probs[i] > sb->probs[best])
         best = i;

   return sb->labels[best];
}

const char *sb_detect(struct sabir *sb, const void *text, size_t len)
{
   sb_init(sb);
   sb_process(sb, text, len < SSIZE_MAX ? len : SSIZE_MAX);
   return sb_finish(sb);
}
