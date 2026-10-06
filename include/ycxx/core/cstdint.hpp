// libycxx core: <cstdint> types and macros, defined from compiler-predefined macros only.
#pragma once

#include <ycxx/config.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {
using int8_t = __INT8_TYPE__;
using int16_t = __INT16_TYPE__;
using int32_t = __INT32_TYPE__;
using int64_t = __INT64_TYPE__;
using uint8_t = __UINT8_TYPE__;
using uint16_t = __UINT16_TYPE__;
using uint32_t = __UINT32_TYPE__;
using uint64_t = __UINT64_TYPE__;

using int_least8_t = __INT_LEAST8_TYPE__;
using int_least16_t = __INT_LEAST16_TYPE__;
using int_least32_t = __INT_LEAST32_TYPE__;
using int_least64_t = __INT_LEAST64_TYPE__;
using uint_least8_t = __UINT_LEAST8_TYPE__;
using uint_least16_t = __UINT_LEAST16_TYPE__;
using uint_least32_t = __UINT_LEAST32_TYPE__;
using uint_least64_t = __UINT_LEAST64_TYPE__;

using int_fast8_t = __INT_FAST8_TYPE__;
#if _YCXX_FAST16_IS_LONG
using int_fast16_t = long;
using int_fast32_t = long;
#else
using int_fast16_t = __INT_FAST16_TYPE__;
using int_fast32_t = __INT_FAST32_TYPE__;
#endif
using int_fast64_t = __INT_FAST64_TYPE__;
using uint_fast8_t = __UINT_FAST8_TYPE__;
#if _YCXX_FAST16_IS_LONG
using uint_fast16_t = unsigned long;
using uint_fast32_t = unsigned long;
#else
using uint_fast16_t = __UINT_FAST16_TYPE__;
using uint_fast32_t = __UINT_FAST32_TYPE__;
#endif
using uint_fast64_t = __UINT_FAST64_TYPE__;

using intmax_t = __INTMAX_TYPE__;
using uintmax_t = __UINTMAX_TYPE__;
using intptr_t = __INTPTR_TYPE__;
using uintptr_t = __UINTPTR_TYPE__;
} // namespace std

// The same names in the global namespace (as every C++ library's <cstdint> provides them; whether
// they are is unspecified, [headers]/5). Typedefs, not using-declarations, so that the C
// library's <stdint.h>, which typedefs the same names to the same types, may be included before
// or after: redeclaring a typedef-name as the same type is allowed ([dcl.typedef]/3).
typedef ::std::int8_t int8_t;
typedef ::std::int16_t int16_t;
typedef ::std::int32_t int32_t;
typedef ::std::int64_t int64_t;
typedef ::std::uint8_t uint8_t;
typedef ::std::uint16_t uint16_t;
typedef ::std::uint32_t uint32_t;
typedef ::std::uint64_t uint64_t;
typedef ::std::int_least8_t int_least8_t;
typedef ::std::int_least16_t int_least16_t;
typedef ::std::int_least32_t int_least32_t;
typedef ::std::int_least64_t int_least64_t;
typedef ::std::uint_least8_t uint_least8_t;
typedef ::std::uint_least16_t uint_least16_t;
typedef ::std::uint_least32_t uint_least32_t;
typedef ::std::uint_least64_t uint_least64_t;
typedef ::std::int_fast8_t int_fast8_t;
typedef ::std::int_fast16_t int_fast16_t;
typedef ::std::int_fast32_t int_fast32_t;
typedef ::std::int_fast64_t int_fast64_t;
typedef ::std::uint_fast8_t uint_fast8_t;
typedef ::std::uint_fast16_t uint_fast16_t;
typedef ::std::uint_fast32_t uint_fast32_t;
typedef ::std::uint_fast64_t uint_fast64_t;
typedef ::std::intmax_t intmax_t;
typedef ::std::uintmax_t uintmax_t;
typedef ::std::intptr_t intptr_t;
typedef ::std::uintptr_t uintptr_t;


// The C macros. Guarded individually so that mixing with the C library's <stdint.h> is benign.
#ifndef INT8_MIN
#  define INT8_MIN (-__INT8_MAX__ - 1)
#  define INT16_MIN (-__INT16_MAX__ - 1)
#  define INT32_MIN (-__INT32_MAX__ - 1)
#  define INT64_MIN (-__INT64_MAX__ - 1)
#  define INT8_MAX __INT8_MAX__
#  define INT16_MAX __INT16_MAX__
#  define INT32_MAX __INT32_MAX__
#  define INT64_MAX __INT64_MAX__
#  define UINT8_MAX __UINT8_MAX__
#  define UINT16_MAX __UINT16_MAX__
#  define UINT32_MAX __UINT32_MAX__
#  define UINT64_MAX __UINT64_MAX__
#endif
#ifndef INT_LEAST8_MIN
#  define INT_LEAST8_MIN (-__INT_LEAST8_MAX__ - 1)
#  define INT_LEAST16_MIN (-__INT_LEAST16_MAX__ - 1)
#  define INT_LEAST32_MIN (-__INT_LEAST32_MAX__ - 1)
#  define INT_LEAST64_MIN (-__INT_LEAST64_MAX__ - 1)
#  define INT_LEAST8_MAX __INT_LEAST8_MAX__
#  define INT_LEAST16_MAX __INT_LEAST16_MAX__
#  define INT_LEAST32_MAX __INT_LEAST32_MAX__
#  define INT_LEAST64_MAX __INT_LEAST64_MAX__
#  define UINT_LEAST8_MAX __UINT_LEAST8_MAX__
#  define UINT_LEAST16_MAX __UINT_LEAST16_MAX__
#  define UINT_LEAST32_MAX __UINT_LEAST32_MAX__
#  define UINT_LEAST64_MAX __UINT_LEAST64_MAX__
#endif
#ifndef INT_FAST8_MIN
#  define INT_FAST8_MIN (-__INT_FAST8_MAX__ - 1)
#  define INT_FAST8_MAX __INT_FAST8_MAX__
#  define UINT_FAST8_MAX __UINT_FAST8_MAX__
#  define INT_FAST64_MIN (-__INT_FAST64_MAX__ - 1)
#  define INT_FAST64_MAX __INT_FAST64_MAX__
#  define UINT_FAST64_MAX __UINT_FAST64_MAX__
#  if _YCXX_FAST16_IS_LONG
#    define INT_FAST16_MIN (-__INT64_MAX__ - 1)
#    define INT_FAST16_MAX __INT64_MAX__
#    define UINT_FAST16_MAX __UINT64_MAX__
#    define INT_FAST32_MIN (-__INT64_MAX__ - 1)
#    define INT_FAST32_MAX __INT64_MAX__
#    define UINT_FAST32_MAX __UINT64_MAX__
#  else
#    define INT_FAST16_MIN (-__INT_FAST16_MAX__ - 1)
#    define INT_FAST16_MAX __INT_FAST16_MAX__
#    define UINT_FAST16_MAX __UINT_FAST16_MAX__
#    define INT_FAST32_MIN (-__INT_FAST32_MAX__ - 1)
#    define INT_FAST32_MAX __INT_FAST32_MAX__
#    define UINT_FAST32_MAX __UINT_FAST32_MAX__
#  endif
#endif
#ifndef INTPTR_MIN
#  define INTPTR_MIN (-__INTPTR_MAX__ - 1)
#  define INTPTR_MAX __INTPTR_MAX__
#  define UINTPTR_MAX __UINTPTR_MAX__
#endif
#ifndef INTMAX_MIN
#  define INTMAX_MIN (-__INTMAX_MAX__ - 1)
#  define INTMAX_MAX __INTMAX_MAX__
#  define UINTMAX_MAX __UINTMAX_MAX__
#endif
#ifndef PTRDIFF_MIN
#  define PTRDIFF_MIN (-__PTRDIFF_MAX__ - 1)
#  define PTRDIFF_MAX __PTRDIFF_MAX__
#endif
#ifndef SIZE_MAX
#  define SIZE_MAX __SIZE_MAX__
#endif
#ifndef SIG_ATOMIC_MIN
#  define SIG_ATOMIC_MIN (-__SIG_ATOMIC_MAX__ - 1)
#  define SIG_ATOMIC_MAX __SIG_ATOMIC_MAX__
#endif
#ifndef WCHAR_MIN
#  define WCHAR_MIN __WCHAR_MIN__
#  define WCHAR_MAX __WCHAR_MAX__
#endif
#ifndef WINT_MIN
#  define WINT_MIN __WINT_MIN__
#  define WINT_MAX __WINT_MAX__
#endif
// C23 width macros and version ([cstdint.syn]). Guarded separately: a C library <stdint.h> may
// have defined the limits above but not these.
#ifndef __STDC_VERSION_STDINT_H__
#  define __STDC_VERSION_STDINT_H__ 202311L
#endif
#ifndef INT8_WIDTH
#  define INT8_WIDTH 8
#  define INT16_WIDTH 16
#  define INT32_WIDTH 32
#  define INT64_WIDTH 64
#  define UINT8_WIDTH 8
#  define UINT16_WIDTH 16
#  define UINT32_WIDTH 32
#  define UINT64_WIDTH 64
#endif
#ifndef INT_LEAST8_WIDTH
#  define INT_LEAST8_WIDTH __INT_LEAST8_WIDTH__
#  define INT_LEAST16_WIDTH __INT_LEAST16_WIDTH__
#  define INT_LEAST32_WIDTH __INT_LEAST32_WIDTH__
#  define INT_LEAST64_WIDTH __INT_LEAST64_WIDTH__
#  define UINT_LEAST8_WIDTH __INT_LEAST8_WIDTH__
#  define UINT_LEAST16_WIDTH __INT_LEAST16_WIDTH__
#  define UINT_LEAST32_WIDTH __INT_LEAST32_WIDTH__
#  define UINT_LEAST64_WIDTH __INT_LEAST64_WIDTH__
#endif
#ifndef INT_FAST8_WIDTH
#  define INT_FAST8_WIDTH __INT_FAST8_WIDTH__
#  define INT_FAST64_WIDTH __INT_FAST64_WIDTH__
#  define UINT_FAST8_WIDTH __INT_FAST8_WIDTH__
#  define UINT_FAST64_WIDTH __INT_FAST64_WIDTH__
#  if _YCXX_FAST16_IS_LONG
#    define INT_FAST16_WIDTH __LONG_WIDTH__
#    define INT_FAST32_WIDTH __LONG_WIDTH__
#    define UINT_FAST16_WIDTH __LONG_WIDTH__
#    define UINT_FAST32_WIDTH __LONG_WIDTH__
#  else
#    define INT_FAST16_WIDTH __INT_FAST16_WIDTH__
#    define INT_FAST32_WIDTH __INT_FAST32_WIDTH__
#    define UINT_FAST16_WIDTH __INT_FAST16_WIDTH__
#    define UINT_FAST32_WIDTH __INT_FAST32_WIDTH__
#  endif
#endif
#ifndef INTPTR_WIDTH
#  define INTPTR_WIDTH __INTPTR_WIDTH__
#  define UINTPTR_WIDTH __INTPTR_WIDTH__
#  define INTMAX_WIDTH __INTMAX_WIDTH__
#  define UINTMAX_WIDTH __INTMAX_WIDTH__
#  define PTRDIFF_WIDTH __PTRDIFF_WIDTH__
#  define SIG_ATOMIC_WIDTH __SIG_ATOMIC_WIDTH__
#  define SIZE_WIDTH __SIZE_WIDTH__
#  define WCHAR_WIDTH __WCHAR_WIDTH__
#  define WINT_WIDTH __WINT_WIDTH__
#endif
#ifndef INT8_C
#  define INT8_C(c) __INT8_C(c)
#  define INT16_C(c) __INT16_C(c)
#  define INT32_C(c) __INT32_C(c)
#  define INT64_C(c) __INT64_C(c)
#  define UINT8_C(c) __UINT8_C(c)
#  define UINT16_C(c) __UINT16_C(c)
#  define UINT32_C(c) __UINT32_C(c)
#  define UINT64_C(c) __UINT64_C(c)
#  define INTMAX_C(c) __INTMAX_C(c)
#  define UINTMAX_C(c) __UINTMAX_C(c)
#endif
#ifndef INT8_WIDTH
#  define INT8_WIDTH 8
#  define INT16_WIDTH 16
#  define INT32_WIDTH 32
#  define INT64_WIDTH 64
#  define UINT8_WIDTH 8
#  define UINT16_WIDTH 16
#  define UINT32_WIDTH 32
#  define UINT64_WIDTH 64
#endif

