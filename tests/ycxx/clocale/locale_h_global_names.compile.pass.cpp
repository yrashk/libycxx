// [support.c.headers.other]/1: the C headers <locale.h>, <signal.h>, <setjmp.h>, <stdarg.h> and
// <fenv.h> place in the global namespace each name their <cname> header places in std:
//   [clocale.syn]: lconv, setlocale, localeconv;
//   [csignal.syn]: sig_atomic_t, signal, raise;
//   [csetjmp.syn]: jmp_buf, longjmp;
//   [cstdarg.syn]: va_list;
//   [cfenv.syn]: fenv_t, fexcept_t, feclearexcept, fegetexceptflag, feraiseexcept,
//     fesetexceptflag, fetestexcept, fegetround, fesetround, fegetenv, feholdexcept, fesetenv,
//     feupdateenv.
// Only these .h headers are included.
#include <fenv.h>
#include <locale.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>

template <class A, class B>
constexpr bool same = __is_same(A, B);

using ::lconv;
using ::localeconv;
using ::setlocale;
static_assert(same<decltype(::localeconv()), ::lconv*>);
static_assert(LC_ALL != LC_NUMERIC || LC_ALL == LC_NUMERIC);

using ::raise;
using ::sig_atomic_t;
using ::signal;
static_assert(same<decltype(::raise(SIGINT)), int>);

using ::jmp_buf;
using ::longjmp;

using ::va_list;

using ::feclearexcept;
using ::fegetenv;
using ::fegetexceptflag;
using ::fegetround;
using ::feholdexcept;
using ::fenv_t;
using ::feraiseexcept;
using ::fesetenv;
using ::fesetexceptflag;
using ::fesetround;
using ::fetestexcept;
using ::feupdateenv;
using ::fexcept_t;

int main() { return ::fegetround() == ::fegetround() ? 0 : 1; }
