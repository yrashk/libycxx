// [cstdint.syn]/2: <cstdint> "defines all types and macros the same as the C standard library
// header <stdint.h>, except that the types intptr_t and uintptr_t and the macros INTPTR_MIN,
// INTPTR_MAX, and UINTPTR_MAX are always defined." /3: "if an implementation defines integer
// types with the corresponding width and no padding bits, it declares the corresponding
// typedef-names. Each of the macros listed in this subclause is defined if and only if the
// implementation declares the corresponding typedef-name."
// C 7.22.2: each MIN/MAX macro is "an integer constant expression suitable for use in #if
// preprocessing directives" whose type is that of "an expression of the corresponding type
// converted according to the integer promotions", with the limits of that type; WIDTH macros
// give the width. C 7.22.3: PTRDIFF_*, SIZE_*, SIG_ATOMIC_*, WCHAR_*, WINT_*. C 7.22.4: INTN_C
// and UINTN_C expand to an integer constant expression of type int_leastN_t/uint_leastN_t
// after promotion; INTMAX_C/UINTMAX_C to intmax_t/uintmax_t.
// [cstdint.syn]: "#define __STDC_VERSION_STDINT_H__ 202311L".
#include <cstdint>
#include <csignal>
#include <cwchar>
#include <cstddef>
#include <limits>
#include <type_traits>

#if !defined(__STDC_VERSION_STDINT_H__) || __STDC_VERSION_STDINT_H__ != 202311L
#  error "__STDC_VERSION_STDINT_H__ must be 202311L"
#endif

template <class T>
using P = decltype(+T());  // the promoted type
template <class T>
constexpr int width = std::numeric_limits<T>::digits + std::numeric_limits<T>::is_signed;
#define SAME_TYPE(M, T) static_assert(std::is_same_v<std::remove_cv_t<decltype(M)>, P<T>>, #M " type")
#define SMIN(M, T) static_assert(M == std::numeric_limits<T>::min(), #M); SAME_TYPE(M, T)
#define SMAX(M, T) static_assert(M == std::numeric_limits<T>::max(), #M); SAME_TYPE(M, T)
#define SWIDTH(M, T) static_assert(M == width<T>, #M)

#if !defined(INT8_MIN) || !defined(INT8_MAX) || !defined(UINT8_MAX) || !defined(INT8_WIDTH) || !defined(UINT8_WIDTH)
#  error "missing one of INT8_MIN INT8_MAX UINT8_MAX INT8_WIDTH UINT8_WIDTH"
#endif
SMIN(INT8_MIN, std::int8_t);
SMAX(INT8_MAX, std::int8_t);
SMAX(UINT8_MAX, std::uint8_t);
SWIDTH(INT8_WIDTH, std::int8_t);
SWIDTH(UINT8_WIDTH, std::uint8_t);
#if !defined(INT_LEAST8_MIN) || !defined(INT_LEAST8_MAX) || !defined(UINT_LEAST8_MAX) || !defined(INT_LEAST8_WIDTH) || !defined(UINT_LEAST8_WIDTH)
#  error "missing one of INT_LEAST8_MIN INT_LEAST8_MAX UINT_LEAST8_MAX INT_LEAST8_WIDTH UINT_LEAST8_WIDTH"
#endif
SMIN(INT_LEAST8_MIN, std::int_least8_t);
SMAX(INT_LEAST8_MAX, std::int_least8_t);
SMAX(UINT_LEAST8_MAX, std::uint_least8_t);
SWIDTH(INT_LEAST8_WIDTH, std::int_least8_t);
SWIDTH(UINT_LEAST8_WIDTH, std::uint_least8_t);
#if !defined(INT_FAST8_MIN) || !defined(INT_FAST8_MAX) || !defined(UINT_FAST8_MAX) || !defined(INT_FAST8_WIDTH) || !defined(UINT_FAST8_WIDTH)
#  error "missing one of INT_FAST8_MIN INT_FAST8_MAX UINT_FAST8_MAX INT_FAST8_WIDTH UINT_FAST8_WIDTH"
#endif
SMIN(INT_FAST8_MIN, std::int_fast8_t);
SMAX(INT_FAST8_MAX, std::int_fast8_t);
SMAX(UINT_FAST8_MAX, std::uint_fast8_t);
SWIDTH(INT_FAST8_WIDTH, std::int_fast8_t);
SWIDTH(UINT_FAST8_WIDTH, std::uint_fast8_t);
SAME_TYPE(INT8_C(1), std::int_least8_t);
SAME_TYPE(UINT8_C(1), std::uint_least8_t);
static_assert(INT8_C(5) == 5 && UINT8_C(5) == 5u);
#if !defined(INT16_MIN) || !defined(INT16_MAX) || !defined(UINT16_MAX) || !defined(INT16_WIDTH) || !defined(UINT16_WIDTH)
#  error "missing one of INT16_MIN INT16_MAX UINT16_MAX INT16_WIDTH UINT16_WIDTH"
#endif
SMIN(INT16_MIN, std::int16_t);
SMAX(INT16_MAX, std::int16_t);
SMAX(UINT16_MAX, std::uint16_t);
SWIDTH(INT16_WIDTH, std::int16_t);
SWIDTH(UINT16_WIDTH, std::uint16_t);
#if !defined(INT_LEAST16_MIN) || !defined(INT_LEAST16_MAX) || !defined(UINT_LEAST16_MAX) || !defined(INT_LEAST16_WIDTH) || !defined(UINT_LEAST16_WIDTH)
#  error "missing one of INT_LEAST16_MIN INT_LEAST16_MAX UINT_LEAST16_MAX INT_LEAST16_WIDTH UINT_LEAST16_WIDTH"
#endif
SMIN(INT_LEAST16_MIN, std::int_least16_t);
SMAX(INT_LEAST16_MAX, std::int_least16_t);
SMAX(UINT_LEAST16_MAX, std::uint_least16_t);
SWIDTH(INT_LEAST16_WIDTH, std::int_least16_t);
SWIDTH(UINT_LEAST16_WIDTH, std::uint_least16_t);
#if !defined(INT_FAST16_MIN) || !defined(INT_FAST16_MAX) || !defined(UINT_FAST16_MAX) || !defined(INT_FAST16_WIDTH) || !defined(UINT_FAST16_WIDTH)
#  error "missing one of INT_FAST16_MIN INT_FAST16_MAX UINT_FAST16_MAX INT_FAST16_WIDTH UINT_FAST16_WIDTH"
#endif
SMIN(INT_FAST16_MIN, std::int_fast16_t);
SMAX(INT_FAST16_MAX, std::int_fast16_t);
SMAX(UINT_FAST16_MAX, std::uint_fast16_t);
SWIDTH(INT_FAST16_WIDTH, std::int_fast16_t);
SWIDTH(UINT_FAST16_WIDTH, std::uint_fast16_t);
SAME_TYPE(INT16_C(1), std::int_least16_t);
SAME_TYPE(UINT16_C(1), std::uint_least16_t);
static_assert(INT16_C(5) == 5 && UINT16_C(5) == 5u);
#if !defined(INT32_MIN) || !defined(INT32_MAX) || !defined(UINT32_MAX) || !defined(INT32_WIDTH) || !defined(UINT32_WIDTH)
#  error "missing one of INT32_MIN INT32_MAX UINT32_MAX INT32_WIDTH UINT32_WIDTH"
#endif
SMIN(INT32_MIN, std::int32_t);
SMAX(INT32_MAX, std::int32_t);
SMAX(UINT32_MAX, std::uint32_t);
SWIDTH(INT32_WIDTH, std::int32_t);
SWIDTH(UINT32_WIDTH, std::uint32_t);
#if !defined(INT_LEAST32_MIN) || !defined(INT_LEAST32_MAX) || !defined(UINT_LEAST32_MAX) || !defined(INT_LEAST32_WIDTH) || !defined(UINT_LEAST32_WIDTH)
#  error "missing one of INT_LEAST32_MIN INT_LEAST32_MAX UINT_LEAST32_MAX INT_LEAST32_WIDTH UINT_LEAST32_WIDTH"
#endif
SMIN(INT_LEAST32_MIN, std::int_least32_t);
SMAX(INT_LEAST32_MAX, std::int_least32_t);
SMAX(UINT_LEAST32_MAX, std::uint_least32_t);
SWIDTH(INT_LEAST32_WIDTH, std::int_least32_t);
SWIDTH(UINT_LEAST32_WIDTH, std::uint_least32_t);
#if !defined(INT_FAST32_MIN) || !defined(INT_FAST32_MAX) || !defined(UINT_FAST32_MAX) || !defined(INT_FAST32_WIDTH) || !defined(UINT_FAST32_WIDTH)
#  error "missing one of INT_FAST32_MIN INT_FAST32_MAX UINT_FAST32_MAX INT_FAST32_WIDTH UINT_FAST32_WIDTH"
#endif
SMIN(INT_FAST32_MIN, std::int_fast32_t);
SMAX(INT_FAST32_MAX, std::int_fast32_t);
SMAX(UINT_FAST32_MAX, std::uint_fast32_t);
SWIDTH(INT_FAST32_WIDTH, std::int_fast32_t);
SWIDTH(UINT_FAST32_WIDTH, std::uint_fast32_t);
SAME_TYPE(INT32_C(1), std::int_least32_t);
SAME_TYPE(UINT32_C(1), std::uint_least32_t);
static_assert(INT32_C(5) == 5 && UINT32_C(5) == 5u);
#if !defined(INT64_MIN) || !defined(INT64_MAX) || !defined(UINT64_MAX) || !defined(INT64_WIDTH) || !defined(UINT64_WIDTH)
#  error "missing one of INT64_MIN INT64_MAX UINT64_MAX INT64_WIDTH UINT64_WIDTH"
#endif
SMIN(INT64_MIN, std::int64_t);
SMAX(INT64_MAX, std::int64_t);
SMAX(UINT64_MAX, std::uint64_t);
SWIDTH(INT64_WIDTH, std::int64_t);
SWIDTH(UINT64_WIDTH, std::uint64_t);
#if !defined(INT_LEAST64_MIN) || !defined(INT_LEAST64_MAX) || !defined(UINT_LEAST64_MAX) || !defined(INT_LEAST64_WIDTH) || !defined(UINT_LEAST64_WIDTH)
#  error "missing one of INT_LEAST64_MIN INT_LEAST64_MAX UINT_LEAST64_MAX INT_LEAST64_WIDTH UINT_LEAST64_WIDTH"
#endif
SMIN(INT_LEAST64_MIN, std::int_least64_t);
SMAX(INT_LEAST64_MAX, std::int_least64_t);
SMAX(UINT_LEAST64_MAX, std::uint_least64_t);
SWIDTH(INT_LEAST64_WIDTH, std::int_least64_t);
SWIDTH(UINT_LEAST64_WIDTH, std::uint_least64_t);
#if !defined(INT_FAST64_MIN) || !defined(INT_FAST64_MAX) || !defined(UINT_FAST64_MAX) || !defined(INT_FAST64_WIDTH) || !defined(UINT_FAST64_WIDTH)
#  error "missing one of INT_FAST64_MIN INT_FAST64_MAX UINT_FAST64_MAX INT_FAST64_WIDTH UINT_FAST64_WIDTH"
#endif
SMIN(INT_FAST64_MIN, std::int_fast64_t);
SMAX(INT_FAST64_MAX, std::int_fast64_t);
SMAX(UINT_FAST64_MAX, std::uint_fast64_t);
SWIDTH(INT_FAST64_WIDTH, std::int_fast64_t);
SWIDTH(UINT_FAST64_WIDTH, std::uint_fast64_t);
SAME_TYPE(INT64_C(1), std::int_least64_t);
SAME_TYPE(UINT64_C(1), std::uint_least64_t);
static_assert(INT64_C(5) == 5 && UINT64_C(5) == 5u);
#if !defined(INTMAX_MIN) || !defined(INTMAX_MAX) || !defined(UINTMAX_MAX) || !defined(INTMAX_WIDTH) || !defined(UINTMAX_WIDTH) || !defined(INTPTR_MIN) || !defined(INTPTR_MAX) || !defined(UINTPTR_MAX) || !defined(INTPTR_WIDTH) || !defined(UINTPTR_WIDTH)
#  error "missing one of INTMAX_MIN INTMAX_MAX UINTMAX_MAX INTMAX_WIDTH UINTMAX_WIDTH INTPTR_MIN INTPTR_MAX UINTPTR_MAX INTPTR_WIDTH UINTPTR_WIDTH"
#endif
#if !defined(PTRDIFF_MIN) || !defined(PTRDIFF_MAX) || !defined(PTRDIFF_WIDTH) || !defined(SIZE_MAX) || !defined(SIZE_WIDTH) || !defined(SIG_ATOMIC_MIN) || !defined(SIG_ATOMIC_MAX) || !defined(SIG_ATOMIC_WIDTH)
#  error "missing one of PTRDIFF_MIN PTRDIFF_MAX PTRDIFF_WIDTH SIZE_MAX SIZE_WIDTH SIG_ATOMIC_MIN SIG_ATOMIC_MAX SIG_ATOMIC_WIDTH"
#endif
#if !defined(WCHAR_MIN) || !defined(WCHAR_MAX) || !defined(WCHAR_WIDTH) || !defined(WINT_MIN) || !defined(WINT_MAX) || !defined(WINT_WIDTH) || !defined(INTMAX_C) || !defined(UINTMAX_C)
#  error "missing one of WCHAR_MIN WCHAR_MAX WCHAR_WIDTH WINT_MIN WINT_MAX WINT_WIDTH INTMAX_C UINTMAX_C"
#endif
SMIN(INTMAX_MIN, std::intmax_t);
SMAX(INTMAX_MAX, std::intmax_t);
SMAX(UINTMAX_MAX, std::uintmax_t);
SWIDTH(INTMAX_WIDTH, std::intmax_t);
SWIDTH(UINTMAX_WIDTH, std::uintmax_t);
SMIN(INTPTR_MIN, std::intptr_t);
SMAX(INTPTR_MAX, std::intptr_t);
SMAX(UINTPTR_MAX, std::uintptr_t);
SWIDTH(INTPTR_WIDTH, std::intptr_t);
SWIDTH(UINTPTR_WIDTH, std::uintptr_t);
SMIN(PTRDIFF_MIN, std::ptrdiff_t);
SMAX(PTRDIFF_MAX, std::ptrdiff_t);
SWIDTH(PTRDIFF_WIDTH, std::ptrdiff_t);
SMAX(SIZE_MAX, std::size_t);
SWIDTH(SIZE_WIDTH, std::size_t);
SMIN(SIG_ATOMIC_MIN, std::sig_atomic_t);
SMAX(SIG_ATOMIC_MAX, std::sig_atomic_t);
SWIDTH(SIG_ATOMIC_WIDTH, std::sig_atomic_t);
SMIN(WCHAR_MIN, wchar_t);
SMAX(WCHAR_MAX, wchar_t);
SWIDTH(WCHAR_WIDTH, wchar_t);
SMIN(WINT_MIN, std::wint_t);
SMAX(WINT_MAX, std::wint_t);
SWIDTH(WINT_WIDTH, std::wint_t);
SAME_TYPE(INTMAX_C(1), std::intmax_t);
SAME_TYPE(UINTMAX_C(1), std::uintmax_t);
static_assert(INTMAX_C(9223372036854775807) == INTMAX_MAX || INTMAX_WIDTH > 64);
static_assert(UINT64_C(18446744073709551615) == UINT64_MAX);

// usable in #if
#if INT8_MAX != 127 || UINT16_MAX != 65535 || INT32_MIN >= 0 || UINT64_MAX == 0 || SIZE_MAX == 0
#  error "macros in #if"
#endif
#if UINTPTR_MAX == 0 || INTPTR_MIN >= 0 || PTRDIFF_MAX <= 0 || INT64_WIDTH != 64
#  error "macros in #if"
#endif
