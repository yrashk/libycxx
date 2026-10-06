// libycxx POSIX platform abstraction layer: the environment's character encoding
// (std::text_encoding::environment). Kept apart from pal.c, in C like it.
#define _GNU_SOURCE
#include <ycxx/pal.h>

#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <string.h>
#if defined(__APPLE__)
/* Darwin declares the *_l functions (nl_langinfo_l) in <xlocale.h>, which declares those of
   <langinfo.h> only when included after it. */
#  include <xlocale.h>
#endif

int __ycxx_pal_environment_encoding(char* __buf, __ycxx_pal_size n) {
  if (n == 0)
    return EINVAL;
  // The codeset of the POSIX locale "" ([text.encoding.members]/14), through a locale object of
  // its own, so setlocale does not affect it. If "" names no valid locale, that of "C".
  const int __saved = errno;
  locale_t __loc = newlocale(LC_CTYPE_MASK, "", (locale_t)0);
  if (__loc == (locale_t)0)
    __loc = newlocale(LC_CTYPE_MASK, "C", (locale_t)0);
  errno = __saved;
  if (__loc == (locale_t)0) {
    __buf[0] = '\0';
    return ENOMEM;
  }
  const char* __cs = nl_langinfo_l(CODESET, __loc);
  size_t __len = __cs ? strlen(__cs) : 0;
  if (__len >= n)
    __len = n - 1;
  if (__len)
    memcpy(__buf, __cs, __len);
  __buf[__len] = '\0';
  freelocale(__loc);
  return 0;
}
