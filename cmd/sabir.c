#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../sabir.h"
#include "cmd.h"

noreturn static void version(void)
{
   const char *msg =
   "Sabir version "SB_VERSION"\n"
   "Copyright (c) 2016 Michaël Meyer"
   ;
   puts(msg);
   exit(EXIT_SUCCESS);
}

static const char *detect(struct sabir *sb, const char *path, size_t buf_size)
{
   FILE *fp = path ? fopen(path, "r") : stdin;
   if (!fp) {
      complain("cannot open '%s':", path);
      return NULL;
   }

   char buf[BUFSIZ];
   size_t size;
   sb_init(sb);
   while ((size = fread(buf, 1, buf_size, fp)))
      sb_feed(sb, buf, size);

   const char *lang = NULL;
   if (ferror(fp))
      complain("cannot read '%s':", path);
   else
      lang = sb_finish(sb);

   if (path)  
      fclose(fp);
   return lang;
}

static int process_one(struct sabir *sb, const char *path, size_t buf_size)
{
   const char *lang = detect(sb, path, buf_size);
   if (!lang)
      return EXIT_FAILURE;

   puts(lang);
   return EXIT_SUCCESS;
}

static int process_many(struct sabir *sb, int nr, char **files, size_t buf_size)
{
   int ret = EXIT_SUCCESS;

   for (int i = 0; i < nr; i++) {
      const char *path = files[i];
      const char *lang = detect(sb, path, buf_size);
      if (!lang)
         ret = EXIT_FAILURE;
      else
         printf("%s:%s\n", path, lang);
   }
   return ret;
}

static int display_langs(struct sabir *sb)
{
   const char *const *langs = sb_langs(sb, NULL);
   while (*langs)
      puts(*langs++);
   
   return EXIT_SUCCESS;
}

/* We do that for testing. */
static size_t get_buf_size(void)
{
   const char *how_much = getenv("SB_BUF_SIZE");
   uintmax_t val;

   if (how_much && sscanf(how_much, "%ju", &val) == 1 && val > 0 && val < BUFSIZ)
      return val;
   return BUFSIZ;
}

int main(int argc, char **argv)
{
   const char *model = SB_PREFIX"/share/sabir/model.sb";
   bool list = false;
   extern bool sb_verbose;
   struct option opts[] = {
      {'m', "model", OPT_STR(model)},
      {'l', "list", OPT_BOOL(list)},
      {'v', "verbose", OPT_BOOL(sb_verbose)},
      {'\0', "version", OPT_FUNC(version)},
      {0},
   };
   const char help[] =
      #include "sabir.ih"
   ;

   parse_options(opts, help, &argc, &argv);

   struct sabir *sb;
   int ret = sb_load(&sb, model);
   if (ret)
      die("cannot load model from '%s': %s", model, sb_strerror(ret));

   if (list)
      ret = display_langs(sb);
   else if (argc <= 1)
      ret = process_one(sb, *argv, get_buf_size());
   else
      ret = process_many(sb, argc, argv, get_buf_size());
   
   sb_dealloc(sb);
   return ret;
}
