// libycxx POSIX platform abstraction layer: the environment's character encoding
// (std::text_encoding::environment). Kept apart from pal.c, in C like it.
#define _GNU_SOURCE
#include <ycxx/pal.h>

#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <string.h>

int ycxx_pal_environment_encoding(char* buf, ycxx_pal_size n) {
  if (n == 0)
    return EINVAL;
  // The codeset of the POSIX locale "" ([text.encoding.members]/14), through a locale object of
  // its own, so setlocale does not affect it. If "" names no valid locale, that of "C".
  const int saved = errno;
  locale_t loc = newlocale(LC_CTYPE_MASK, "", (locale_t)0);
  if (loc == (locale_t)0)
    loc = newlocale(LC_CTYPE_MASK, "C", (locale_t)0);
  errno = saved;
  if (loc == (locale_t)0) {
    buf[0] = '\0';
    return ENOMEM;
  }
  const char* cs = nl_langinfo_l(CODESET, loc);
  size_t len = cs ? strlen(cs) : 0;
  if (len >= n)
    len = n - 1;
  if (len)
    memcpy(buf, cs, len);
  buf[len] = '\0';
  freelocale(loc);
  return 0;
}
