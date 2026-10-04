// [numeric.limits.general]/4: specializations for each arithmetic type with is_specialized
// true; [numeric.limits.members]: min/max are "Equivalent to CHAR_MIN, SHRT_MIN, ...";
// digits is "the number of non-sign bits in the representation"; digits10 is the number of
// base 10 digits representable without change; integer types are exact, bounded, radix 2;
// "Specializations for integer types shall return round_toward_zero"; is_modulo is false for
// signed integer types and (by definition) true for unsigned types that are not promoted.
#include <limits>
#include <climits>
#include <cstdint>
#include <cwchar>

template <class T>
constexpr int bits = sizeof(T) * CHAR_BIT;

// floor(digits * log10(2)) computed exactly enough for digits < 1000
constexpr int digits10_of(int digits) { return static_cast<int>((static_cast<long long>(digits) * 30103) / 100000); }

template <class T, bool Signed, T Min, T Max>
constexpr bool check() {
  using L = std::numeric_limits<T>;
  static_assert(L::is_specialized);
  static_assert(L::min() == Min && L::max() == Max && L::lowest() == Min);
  static_assert(L::is_signed == Signed);
  static_assert(L::digits == bits<T> - (Signed ? 1 : 0));
  static_assert(L::digits10 == digits10_of(L::digits));
  static_assert(L::max_digits10 == 0);
  static_assert(L::is_integer && L::is_exact && L::is_bounded && L::radix == 2);
  if constexpr (Signed) static_assert(!L::is_modulo);  // [numeric.limits.members] Example 1
  if constexpr (!Signed && sizeof(T) >= sizeof(int)) static_assert(L::is_modulo);  // unsigned arithmetic wraps
  static_assert(L::epsilon() == 0 && L::round_error() == 0);
  static_assert(!L::has_infinity && !L::has_quiet_NaN && !L::has_signaling_NaN);
  static_assert(L::min_exponent == 0 && L::max_exponent10 == 0);
  static_assert(!L::is_iec559 && !L::tinyness_before);
  static_assert(L::round_style == std::round_toward_zero);
  return true;
}

static_assert(check<signed char, true, SCHAR_MIN, SCHAR_MAX>());
static_assert(check<unsigned char, false, 0, UCHAR_MAX>());
static_assert(check<char, (CHAR_MIN < 0), CHAR_MIN, CHAR_MAX>());
static_assert(check<short, true, SHRT_MIN, SHRT_MAX>());
static_assert(check<unsigned short, false, 0, USHRT_MAX>());
static_assert(check<int, true, INT_MIN, INT_MAX>());
static_assert(check<unsigned, false, 0, UINT_MAX>());
static_assert(check<long, true, LONG_MIN, LONG_MAX>());
static_assert(check<unsigned long, false, 0, ULONG_MAX>());
static_assert(check<long long, true, LLONG_MIN, LLONG_MAX>());
static_assert(check<unsigned long long, false, 0, ULLONG_MAX>());
static_assert(check<wchar_t, (WCHAR_MIN < 0), WCHAR_MIN, WCHAR_MAX>());
static_assert(check<char8_t, false, 0, static_cast<char8_t>(UCHAR_MAX)>());
static_assert(check<char16_t, false, 0, static_cast<char16_t>(UINT_LEAST16_MAX)>());
static_assert(check<char32_t, false, 0, static_cast<char32_t>(UINT_LEAST32_MAX)>());

// a couple of well-known exact values
static_assert(std::numeric_limits<std::int32_t>::digits10 == 9);
static_assert(std::numeric_limits<std::uint64_t>::digits10 == 19);
static_assert(std::numeric_limits<std::int8_t>::digits10 == 2);
