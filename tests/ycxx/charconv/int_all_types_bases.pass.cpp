// Integer to_chars / from_chars for every integer type (char, signed char, unsigned char,
// short ... unsigned long long; not bool, [charconv.syn]: "for all cv-unqualified signed and
// unsigned integer types and char") and every base 2..36:
// [charconv.to.chars]/7: "value is converted to a string of digits in the given base (with no
// redundant leading zeroes). Digits in the range 10..35 (inclusive) are represented as
// lowercase characters a..z. If value is less than zero, the representation starts with '-'."
// [charconv.from.chars]/5 (pattern of strtol), /1: on success value holds the parsed value
// and ptr points at the first character not matching the pattern; "Otherwise, if the parsed
// value is not in the range representable by the type of value, value is unmodified and the
// member ec of the return value is equal to errc::result_out_of_range" (ptr still past the
// matched pattern). Checked: exhaustively for 8- and 16-bit types, at the boundaries for the
// others; the strings for min - 1 and max + 1 are built independently with 128-bit
// arithmetic. The integer overloads are constexpr ([charconv.syn]), checked with static_assert.
#include <charconv>
#include <climits>
#include <cstddef>
#include <limits>
#include <system_error>
#include <type_traits>
#include "check.hpp"

using u128 = unsigned __int128;

// Independent reference: digits of |v| (v given as sign + 128-bit magnitude) in base b.
constexpr int ref_digits(bool neg, u128 mag, int base, char* out) {
  char tmp[140];
  int n = 0;
  do {
    int d = static_cast<int>(mag % static_cast<unsigned>(base));
    tmp[n++] = static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10);
    mag /= static_cast<unsigned>(base);
  } while (mag != 0);
  int k = 0;
  if (neg) out[k++] = '-';
  while (n > 0) out[k++] = tmp[--n];
  return k;
}

template <class T>
constexpr u128 magnitude(T v) {
  if constexpr (std::is_signed_v<T>) {
    if (v < 0) return static_cast<u128>(-(static_cast<__int128>(v)));
  }
  return static_cast<u128>(v);
}

template <class T>
constexpr bool one(T v, int base) {
  char ref[140], buf[140];
  int n = ref_digits(v < 0, magnitude(v), base, ref);
  auto w = std::to_chars(buf, buf + sizeof buf, v, base);
  if (w.ec != std::errc{} || w.ptr != buf + n) return false;
  for (int i = 0; i < n; ++i)
    if (buf[i] != ref[i]) return false;
  // exactly enough room works, one less fails with value_too_large and ptr == last
  auto w2 = std::to_chars(buf, buf + n, v, base);
  if (w2.ec != std::errc{} || w2.ptr != buf + n) return false;
  auto w3 = std::to_chars(buf, buf + n - 1, v, base);
  if (w3.ec != std::errc::value_too_large || w3.ptr != buf + n - 1) return false;
  // parse back, followed by a character outside the pattern for every base
  ref[n] = '!';
  T back = static_cast<T>(v ^ 1);
  auto r = std::from_chars(ref, ref + n + 1, back, base);
  return r.ec == std::errc{} && r.ptr == ref + n && back == v;
}

// The string of a value one past the range of T must give result_out_of_range, value
// unmodified, ptr past all of the digits.
template <class T>
constexpr bool out_of_range(bool neg, u128 mag, int base) {
  char s[140];
  int n = ref_digits(neg, mag, base, s);
  s[n] = 'z' + 1;  // '{', not a digit in any base
  T v = static_cast<T>(42);
  auto r = std::from_chars(s, s + n + 1, v, base);
  return r.ec == std::errc::result_out_of_range && r.ptr == s + n && v == static_cast<T>(42);
}

template <class T>
constexpr bool boundaries(int base) {
  using L = std::numeric_limits<T>;
  const T vals[] = {T(0), T(1), T(L::max()), T(L::max() - 1), T(L::min()), T(L::min() + 1), T(L::max() / 2),
                    T(static_cast<T>(base)), T(static_cast<T>(base - 1)), T(static_cast<T>(base + 1))};
  for (T v : vals)
    if (!one(v, base)) return false;
  if constexpr (std::is_signed_v<T>)
    if (!one(T(-1), base) || !one(T(-base), base)) return false;
  // powers of the base and their neighbours
  u128 p = 1;
  while (p <= static_cast<u128>(L::max())) {
    if (!one(static_cast<T>(p), base) || !one(static_cast<T>(p - 1), base)) return false;
    if constexpr (std::is_signed_v<T>)
      if (!one(static_cast<T>(-static_cast<__int128>(p)), base)) return false;
    p *= static_cast<unsigned>(base);
  }
  if (!out_of_range<T>(false, static_cast<u128>(L::max()) + 1, base)) return false;
  if (!out_of_range<T>(false, static_cast<u128>(L::max()) * 37 + 5, base)) return false;
  if constexpr (std::is_signed_v<T>) {
    if (!out_of_range<T>(true, magnitude(L::min()) + 1, base)) return false;
  } else {
    // '-' is not part of the pattern for unsigned types: nothing matches
    char s[] = "-1";
    T v = 7;
    auto r = std::from_chars(s, s + 2, v, base);
    if (r.ec != std::errc::invalid_argument || r.ptr != s || v != 7) return false;
  }
  return true;
}

template <class T>
constexpr bool all_bases() {
  for (int base = 2; base <= 36; ++base)
    if (!boundaries<T>(base)) return false;
  return true;
}

template <class T>
bool exhaustive() {
  for (int base = 2; base <= 36; ++base)
    for (long long x = std::numeric_limits<T>::min(); x <= std::numeric_limits<T>::max(); ++x)
      if (!one(static_cast<T>(x), base)) return false;
  return true;
}

// constant evaluation: a selection of bases per type (one static_assert each, to stay within
// the compilers' constexpr step limits); all bases at run time below.
#define CONSTEXPR_BASES(T)                                                                 \
  static_assert(boundaries<T>(2));                                                           \
  static_assert(boundaries<T>(3));                                                           \
  static_assert(boundaries<T>(10));                                                          \
  static_assert(boundaries<T>(16));                                                          \
  static_assert(boundaries<T>(36));
CONSTEXPR_BASES(char)
CONSTEXPR_BASES(signed char)
CONSTEXPR_BASES(unsigned char)
CONSTEXPR_BASES(short)
CONSTEXPR_BASES(unsigned short)
CONSTEXPR_BASES(int)
CONSTEXPR_BASES(unsigned)
CONSTEXPR_BASES(long)
CONSTEXPR_BASES(unsigned long)
CONSTEXPR_BASES(long long)
CONSTEXPR_BASES(unsigned long long)

int main() {
  CHECK(all_bases<char>());
  CHECK(all_bases<signed char>());
  CHECK(all_bases<unsigned char>());
  CHECK(all_bases<short>());
  CHECK(all_bases<unsigned short>());
  CHECK(all_bases<int>());
  CHECK(all_bases<unsigned>());
  CHECK(all_bases<long>());
  CHECK(all_bases<unsigned long>());
  CHECK(all_bases<long long>());
  CHECK(all_bases<unsigned long long>());
  CHECK(exhaustive<char>());
  CHECK(exhaustive<signed char>());
  CHECK(exhaustive<unsigned char>());
  CHECK(exhaustive<short>());
  CHECK(exhaustive<unsigned short>());
  return 0;
}
