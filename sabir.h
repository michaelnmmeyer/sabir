#ifndef SABIR_H
#define SABIR_H

/* This must be linked against the utf8proc library. Its source code is at:
 * https://github.com/JuliaLang/utf8proc
 */

#define SB_VERSION "0.2"

#include <stddef.h>

enum {
   SB_OK,      /* No error. */
   SB_EOPEN,   /* Cannot open model file. */
   SB_EMAGIC,  /* Not a model file. */
   SB_EMODEL,  /* Invalid model file. */
   SB_EIO,     /* I/O error. */
   SB_ENOMEM,  /* Out of memory. */
};

/* Returns a string describing an error code. */
const char *sb_strerror(int err);

struct sabir;

/* Loads a language detection model from a file.
 * On success, makes the provided structure pointer point to the loaded model,
 * and returns SB_OK. Otherwise, makes it point to NULL, and return an error
 * code.
 */
int sb_load(struct sabir **, const char *path);

/* Deallocates a model. */
void sb_dealloc(struct sabir *);

/* Returns the list of languages supported by a model.
 * The returned array is lexicographically sorted and NULL-terminated. It points
 * to the model's internals, and should then not be used after the model is
 * deallocated. If "nr" is not NULL, fills it with the number of supported
 * languages. 
 */
const char *const *sb_langs(struct sabir *, size_t *nr);

/* Classifies a UTF-8 text chunk.
 * The returned pointer points to this object's internals. It should then not
 * be accessed after the model is deallocated.
 * This always returns a value, whether or not the text to classify is written
 * in one of the languages supported by this model.
 */
const char *sb_detect(struct sabir *, const void *text, size_t len);

/* Low-level classification interface, useful when the input text is read from
 * a stream. The calling procedure must be as follows:
 *   1. Call sb_init() to (re)initialize the classifier state.
 *   2. Call sb_feed() one or more times with pieces of the text to classify.
 *      It is assumed that these chunks are contiguous. They need not start or
 *      end on valid UTF-8 boundaries.
 *   3. Call sb_finish() once enough data has been gathered to obtain the best
 *      matching language.
 */
void sb_init(struct sabir *);
void sb_feed(struct sabir *, const void *chunk, size_t len);
const char *sb_finish(struct sabir *);

#endif
