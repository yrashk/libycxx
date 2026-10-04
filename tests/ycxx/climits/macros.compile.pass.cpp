// [climits.syn]: BOOL_WIDTH, CHAR_BIT, CHAR_WIDTH, SCHAR_WIDTH, UCHAR_WIDTH, USHRT_WIDTH,
// SHRT_WIDTH, UINT_WIDTH, INT_WIDTH, ULONG_WIDTH, LONG_WIDTH, ULLONG_WIDTH, LLONG_WIDTH,
// SCHAR_MIN/MAX, UCHAR_MAX, CHAR_MIN/MAX, MB_LEN_MAX, SHRT_MIN/MAX, USHRT_MAX, INT_MIN/MAX,
// UINT_MAX, LONG_MIN/MAX, ULONG_MAX, LLONG_MIN/MAX, ULLONG_MAX. /1: "defines all macros the same
// as the C standard library header <limits.h>, except that it does not define the macro
// BITINT_MAXWIDTH." Note 1: "Except for the WIDTH macros, CHAR_BIT, and MB_LEN_MAX, a macro
// referring to an integer type T defines a constant whose type is the promoted type of T".
// C 5.3.5.3.2: the values are the limits of the types and are usable in #if; BOOL_WIDTH is
// "width for an object of type bool" (exactly 1); CHAR_WIDTH, SCHAR_WIDTH and UCHAR_WIDTH are
// CHAR_BIT; MB_LEN_MAX is at least 1.
#include <climits>
#include <limits>
#include <type_traits>

#if !defined(BOOL_WIDTH) || !defined(CHAR_BIT) || !defined(CHAR_WIDTH) || !defined(SCHAR_WIDTH) || \
    !defined(UCHAR_WIDTH) || !defined(USHRT_WIDTH) || !defined(SHRT_WIDTH) || !defined(UINT_WIDTH) || \
    !defined(INT_WIDTH) || !defined(ULONG_WIDTH) || !defined(LONG_WIDTH) || !defined(ULLONG_WIDTH) || \
    !defined(LLONG_WIDTH)
#  error "a WIDTH macro (or CHAR_BIT) is missing"
#endif
#if !defined(SCHAR_MIN) || !defined(SCHAR_MAX) || !defined(UCHAR_MAX) || !defined(CHAR_MIN) || \
    !defined(CHAR_MAX) || !defined(MB_LEN_MAX) || !defined(SHRT_MIN) || !defined(SHRT_MAX) || \
    !defined(USHRT_MAX) || !defined(INT_MIN) || !defined(INT_MAX) || !defined(UINT_MAX) || \
    !defined(LONG_MIN) || !defined(LONG_MAX) || !defined(ULONG_MAX) || !defined(LLONG_MIN) || \
    !defined(LLONG_MAX) || !defined(ULLONG_MAX)
#  error "a MIN/MAX macro is missing"
#endif
#ifdef BITINT_MAXWIDTH
#  error "<climits> does not define BITINT_MAXWIDTH"
#endif
// usable in #if
#if CHAR_BIT < 8 || INT_MAX < 32767 || LLONG_MAX < 9223372036854775807 || UCHAR_MAX < 255
#  error "values"
#endif

template <class T>
using P = decltype(+T());
template <class T>
constexpr int width = std::numeric_limits<T>::digits + std::numeric_limits<T>::is_signed;
#define LIM(M, T, which) \
  static_assert(M == std::numeric_limits<T>::which(), #M); \
  static_assert(std::is_same_v<decltype(M), P<T>>, #M " has the promoted type")

LIM(SCHAR_MIN, signed char, min);
LIM(SCHAR_MAX, signed char, max);
LIM(UCHAR_MAX, unsigned char, max);
LIM(CHAR_MIN, char, min);
LIM(CHAR_MAX, char, max);
LIM(SHRT_MIN, short, min);
LIM(SHRT_MAX, short, max);
LIM(USHRT_MAX, unsigned short, max);
LIM(INT_MIN, int, min);
LIM(INT_MAX, int, max);
LIM(UINT_MAX, unsigned, max);
LIM(LONG_MIN, long, min);
LIM(LONG_MAX, long, max);
LIM(ULONG_MAX, unsigned long, max);
LIM(LLONG_MIN, long long, min);
LIM(LLONG_MAX, long long, max);
LIM(ULLONG_MAX, unsigned long long, max);

static_assert(CHAR_BIT == std::numeric_limits<unsigned char>::digits);
static_assert(CHAR_WIDTH == CHAR_BIT && SCHAR_WIDTH == CHAR_BIT && UCHAR_WIDTH == CHAR_BIT);
static_assert(BOOL_WIDTH == 1);
static_assert(SHRT_WIDTH == width<short> && USHRT_WIDTH == width<unsigned short>);
static_assert(INT_WIDTH == width<int> && UINT_WIDTH == width<unsigned>);
static_assert(LONG_WIDTH == width<long> && ULONG_WIDTH == width<unsigned long>);
static_assert(LLONG_WIDTH == width<long long> && ULLONG_WIDTH == width<unsigned long long>);
static_assert(MB_LEN_MAX >= 1);
