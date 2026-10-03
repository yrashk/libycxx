// libycxx core: <cstdint> types and macros, defined from compiler-predefined macros only.
#ifndef YCXX_CORE_CSTDINT_HPP
#define YCXX_CORE_CSTDINT_HPP

#include <ycxx/config.hpp>

// Clang's predefined fast types do not match glibc's on 64-bit Linux (glibc uses `long` for
// int_fast16_t/int_fast32_t). Match the C library so std::int_fast16_t == ::int_fast16_t.
#if YCXX_COMPILER_CLANG && defined(__gnu_linux__) && __SIZEOF_POINTER__ == 8
#  define YCXX_INT_FAST16_TYPE long
#  define YCXX_INT_FAST32_TYPE long
#  define YCXX_UINT_FAST16_TYPE unsigned long
#  define YCXX_UINT_FAST32_TYPE unsigned long
#  define YCXX_INT_FAST16_WIDTH 64
#  define YCXX_INT_FAST32_WIDTH 64
#else
#  define YCXX_INT_FAST16_TYPE __INT_FAST16_TYPE__
#  define YCXX_INT_FAST32_TYPE __INT_FAST32_TYPE__
#  define YCXX_UINT_FAST16_TYPE __UINT_FAST16_TYPE__
#  define YCXX_UINT_FAST32_TYPE __UINT_FAST32_TYPE__
#  define YCXX_INT_FAST16_WIDTH __INT_FAST16_WIDTH__
#  define YCXX_INT_FAST32_WIDTH __INT_FAST32_WIDTH__
#endif

namespace std {
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
using int_fast16_t = YCXX_INT_FAST16_TYPE;
using int_fast32_t = YCXX_INT_FAST32_TYPE;
using int_fast64_t = __INT_FAST64_TYPE__;
using uint_fast8_t = __UINT_FAST8_TYPE__;
using uint_fast16_t = YCXX_UINT_FAST16_TYPE;
using uint_fast32_t = YCXX_UINT_FAST32_TYPE;
using uint_fast64_t = __UINT_FAST64_TYPE__;

using intmax_t = __INTMAX_TYPE__;
using uintmax_t = __UINTMAX_TYPE__;
using intptr_t = __INTPTR_TYPE__;
using uintptr_t = __UINTPTR_TYPE__;
} // namespace std

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
#  if YCXX_INT_FAST16_WIDTH == 64
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

#endif // YCXX_CORE_CSTDINT_HPP
