// Test-harness shim: the configuration macros libstdc++'s testsuite helpers (testsuite_hooks.h,
// testsuite_iterators.h, testsuite_allocator.h, ...) consult. testsuite_hooks.h includes this
// header first, so every helper sees it.
// This is NOT part of libycxx; it is only on the include path when running the libstdc++ testsuite
// (tests/libstdcxx/lit.cfg.py). Tests that include libstdc++-internal headers or use these macros
// themselves stay skipped (tests/libstdcxx/skip.txt, tests/ycxxlit/libstdcxx_format.py).
#pragma once

// The target (Linux, glibc): what the testsuite's own configuration would detect.
#define _GLIBCXX_HAVE_SYS_STAT_H 1
#define _GLIBCXX_HAVE_UNISTD_H 1
#define _GLIBCXX_HOSTED 1
#define _GLIBCXX_USE_LONG_LONG 1        // testsuite_io.h: the long long facet overloads
#define _GLIBCXX_USE_C99_MATH_FUNCS 1   // testsuite_random.h: binomial/poisson pdfs (lgamma, ...)
#define _GLIBCXX_HAVE_SYMLINK 1         // testsuite_fs.h: otherwise NO_SYMLINKS
#define _GLIBCXX_HAVE_SYS_STATVFS_H 1   // testsuite_fs.h: otherwise NO_SPACE
#define _GLIBCXX_USE_UTIMENSAT 1        // testsuite_fs.h: otherwise NO_LAST_WRITE_TIME
#define _GLIBCXX_HAVE_FCNTL_H 1         // tests that call utimensat include <fcntl.h> under these
#define _GLIBCXX_HAVE_UTIME_H 1

// The language-version macros the helpers put on their declarations. The run is C++26 only, so
// each is the keyword (libstdc++ spells them so that its headers also compile as C++98..C++23).
#define _GLIBCXX_CONSTEXPR constexpr
#define _GLIBCXX14_CONSTEXPR constexpr
#define _GLIBCXX17_CONSTEXPR constexpr
#define _GLIBCXX20_CONSTEXPR constexpr
#define _GLIBCXX23_CONSTEXPR constexpr
#define _GLIBCXX_NOEXCEPT noexcept
#define _GLIBCXX_USE_NOEXCEPT noexcept
// testsuite_allocator.h rethrows in a catch (...) block with it.
#if __cpp_exceptions
#define __throw_exception_again throw
#else
#define __throw_exception_again
#endif

namespace std {
// testsuite_allocator.h (uneq_allocator) skips its bookkeeping during constant evaluation with
// this. Harness-side equivalent of std::is_constant_evaluated() ([meta.const.eval]).
constexpr bool __is_constant_evaluated() noexcept {
  if consteval {
    return true;
  } else {
    return false;
  }
}
} // namespace std
