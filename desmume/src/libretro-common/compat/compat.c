/* Copyright  (C) 2010-2015 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (compat.c).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <ctype.h>

#ifndef HAVE_GETOPT_LONG
#include <string.h>
#include <boolean.h>
#include <stddef.h>
#include <stdlib.h>

#include <retro_miscellaneous.h>
#endif

#include <compat/getopt.h>
#include <compat/strl.h>
#include <compat/strcasestr.h>
#include <compat/posix_string.h>

#include <retro_assert.h>

#ifndef HAVE_GETOPT_LONG
char *optarg;
int optind, opterr, optopt;

static bool is_short_option(const char *str)
{
   return str[0] == '-' && str[1] != '-';
}

static bool is_long_option(const char *str)
{
   return str[0] == '-' && str[1] == '-';
}

static int find_short_index(char * const *argv)
{
   int idx;
   for (idx = 0; argv[idx]; idx++)
   {
      if (is_short_option(argv[idx]))
         return idx;
   }

   return -1;
}

static int find_long_index(char * const *argv)
{
   int idx;
   for (idx = 0; argv[idx]; idx++)
   {
      if (is_long_option(argv[idx]))
         return idx;
   }

   return -1;
}

static int parse_short(const char *optstring, char * const *argv)
{
   bool extra_opt, takes_arg, embedded_arg;
   const char *opt = NULL;
   char        arg = argv[0][1];

   if (arg == ':')
      return '?';

   opt = strchr(optstring, arg);
   if (!opt)
      return '?';

   extra_opt = argv[0][2];
   takes_arg = opt[1] == ':';

   /* If we take an argument, and we see additional characters,
    * this is in fact the argument (i.e. -cfoo is same as -c foo). */
   embedded_arg = extra_opt && takes_arg;

   if (takes_arg)
   {
      if (embedded_arg)
      {
         optarg = argv[0] + 2;
         optind++;
      }
      else
      {
         optarg = argv[1];
         optind += 2;
      }

      return optarg ? opt[0] : '?';
   }
   else if (embedded_arg)
   {
      /* If we see additional characters, 
       * and they don't take arguments, this 
       * means we have multiple flags in one. */
      memmove(&argv[0][1], &argv[0][2], strlen(&argv[0][2]) + 1);
      return opt[0];
   }
   else
   {
      optind++;
      return opt[0];
   }
}

static int parse_long(const struct option *longopts, char * const *argv)
{
   size_t indice;
   const struct option *opt = NULL;
   for (indice = 0; longopts[indice].name; indice++)
   {
      if (!strcmp(longopts[indice].name, &argv[0][2]))
      {
         opt = &longopts[indice];
         break;
      }
   }

   if (!opt)
      return '?';
   
   /* getopt_long has an "optional" arg, but we don't bother with that. */
   if (opt->has_arg && !argv[1])
      return '?';

   if (opt->has_arg)
   {
      optarg = argv[1];
      optind += 2;
   }
   else
      optind++;

   if (opt->flag)
   {
      *opt->flag = opt->val;
      return 0;
   }

   return opt->val;
}

static void shuffle_block(char **begin, char **last, char **end)
{
   ptrdiff_t    len = last - begin;
   const char **tmp = (const char**)calloc(len, sizeof(const char*));

   rarch_assert(tmp);

   memcpy(tmp, begin, len * sizeof(const char*));
   memmove(begin, last, (end - last) * sizeof(const char*));
   memcpy(end - len, tmp, len * sizeof(const char*));

   free(tmp);
}

int getopt_long(int argc, char *argv[],
      const char *optstring, const struct option *longopts, int *longindex)
{
   int short_index, long_index;

   (void)longindex;

   if (optind == 0)
      optind = 1;

   if (argc == 1)
      return -1;

   short_index = find_short_index(&argv[optind]);
   long_index  = find_long_index(&argv[optind]);

   /* We're done here. */
   if (short_index == -1 && long_index == -1)
      return -1;

   /* Reorder argv so that non-options come last.
    * Non-POSIXy, but that's what getopt does by default. */
   if ((short_index > 0) && ((short_index < long_index) || (long_index == -1)))
   {
      shuffle_block(&argv[optind], &argv[optind + short_index], &argv[argc]);
      short_index = 0;
   }
   else if ((long_index > 0) && ((long_index < short_index)
            || (short_index == -1)))
   {
      shuffle_block(&argv[optind], &argv[optind + long_index], &argv[argc]);
      long_index = 0;
   }

   rarch_assert(short_index == 0 || long_index == 0);

   if (short_index == 0)
      return parse_short(optstring, &argv[optind]);
   else if (long_index == 0)
      return parse_long(longopts, &argv[optind]);
   else
      return '?';
}

#endif

#ifndef HAVE_STRCASESTR
/* Pretty much strncasecmp. */
static int casencmp(const char *a, const char *b, size_t n)
{
   size_t i;

   for (i = 0; i < n; i++)
   {
      int a_lower = tolower(a[i]);
      int b_lower = tolower(b[i]);
      if (a_lower != b_lower)
         return a_lower - b_lower;
   }

   return 0;
}

char *strcasestr_rarch__(const char *haystack, const char *needle)
{
   size_t i, hay_len, needle_len, search_off;

   hay_len = strlen(haystack);
   needle_len = strlen(needle);
   if (needle_len > hay_len)
      return NULL;

   search_off = hay_len - needle_len;
   for (i = 0; i <= search_off; i++)
      if (!casencmp(haystack + i, needle, needle_len))
         return (char*)haystack + i;

   return NULL;
}
#endif

#ifndef HAVE_STRL

/* Implementation of strlcpy()/strlcat() based on OpenBSD. */

size_t strlcpy(char *dest, const char *source, size_t size)
{
   size_t src_size = 0;
   size_t n = size;

   if (n)
      while (--n && (*dest++ = *source++)) src_size++;

   if (!n)
   {
      if (size) *dest = '\0';
      while (*source++) src_size++;
   }

   return src_size;
}

size_t strlcat(char *dest, const char *source, size_t size)
{
   size_t len = strlen(dest);

   dest += len;

   if (len > size)
      size = 0;
   else
      size -= len;

   return len + strlcpy(dest, source, size);
}

#endif

#ifdef _WIN32

#undef strcasecmp
#undef strdup
#undef isblank
#undef strtok_r
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <compat/strl.h>

#include <string.h>

int rarch_strcasecmp__(const char *a, const char *b)
{
   while (*a && *b)
   {
      int a_ = tolower(*a);
      int b_ = tolower(*b);

      if (a_ != b_)
         return a_ - b_;

      a++;
      b++;
   }

   return tolower(*a) - tolower(*b);
}

char *rarch_strdup__(const char *orig)
{
   size_t len = strlen(orig) + 1;
   char *ret  = (char*)malloc(len);
   if (!ret)
      return NULL;

   strlcpy(ret, orig, len);
   return ret;
}

int rarch_isblank__(int c)
{
   return (c == ' ') || (c == '\t');
}

char *rarch_strtok_r__(char *str, const char *delim, char **saveptr)
{
   char *first = NULL;
   if (!saveptr || !delim)
      return NULL;

   if (str)
      *saveptr = str;

   do
   {
      char *ptr = NULL;
      first = *saveptr;
      while (*first && strchr(delim, *first))
         *first++ = '\0';

      if (*first == '\0')
         return NULL;

      ptr = first + 1;

      while (*ptr && !strchr(delim, *ptr))
         ptr++;

      *saveptr = ptr + (*ptr ? 1 : 0);
      *ptr     = '\0';
   } while (strlen(first) == 0);

   return first;
}

#endif
