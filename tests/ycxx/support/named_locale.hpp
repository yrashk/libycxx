// Named locales for the own tests: which of a test's locale names the C library has, asked of
// the C library itself (newlocale), not of the library under test. A test that needs a locale
// the machine lacks calls require_locale(), which prints "UNSUPPORTED: <reason>" and exits with
// status 77: the suite's format reports the test UNSUPPORTED rather than FAIL (tests/ycxxlit/
// ycxx_format.py). tools/ci/gen-locales generates the names the tests use on glibc; macOS has
// them installed.
//
// The C library's view of a locale, for comparing the facets with it: in_c_locale(name, f) runs
// f with the calling thread's C locale set to name (uselocale), so localeconv, nl_langinfo,
// strftime and the multibyte functions answer for that locale.
#pragma once

#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <unistd.h> // _exit: <stdlib.h> would clash with check.hpp's abort()
#if __has_include(<xlocale.h>)
#  include <xlocale.h>
#endif

inline bool c_has_locale(const char* name) {
  locale_t l = newlocale(LC_ALL_MASK, name, (locale_t)0);
  if (l == (locale_t)0)
    return false;
  freelocale(l);
  return true;
}

// name, if the C library has it; otherwise the test ends as UNSUPPORTED.
inline const char* require_locale(const char* name) {
  if (!c_has_locale(name)) {
    printf("UNSUPPORTED: named locale %s not installed (tools/ci/gen-locales generates it)\n", name);
    fflush(stdout);
    _exit(77);
  }
  return name;
}

// Runs f() with the calling thread's C locale set to name; returns what f returns.
template <class F>
auto in_c_locale(const char* name, F f) {
  locale_t l = newlocale(LC_ALL_MASK, name, (locale_t)0);
  if (l == (locale_t)0) {
    printf("UNSUPPORTED: named locale %s not installed\n", name);
    fflush(stdout);
    _exit(77);
  }
  locale_t old = uselocale(l);
  struct restore {
    locale_t old, l;
    ~restore() {
      uselocale(old);
      freelocale(l);
    }
  } r{old, l};
  return f();
}
