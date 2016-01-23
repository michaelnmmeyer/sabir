/* Detects the language of a file read from the standard input. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sabir.h"

int main(void)
{
   /* Load our model. */
   struct sabir *sb;
   int ret = sb_load(&sb, "model.sb");
   if (ret) {
      fprintf(stderr, "%s: cannot load model: %s", *argv, sb_strerror(ret));
      return EXIT_FAILURE;
   }

   /* Reinitialize the classifier. */
   sb_init(sb);

   /* Feed text chunks to the classifier. */
   char buf[4096];
   size_t len;
   while ((len = fread(buf, 1, sizeof buf, stdin)))
      sb_feed(sb, buf, len);

   /* Print the best language found. */
   const char *lang = sb_finish(sb);
   puts(lang);

   sb_dealloc(sb);
   return EXIT_SUCCESS;
}
