// [charconv.to.chars]/7: integer to_chars writes "the value of value ... converted to a string of
// digits in the given base (with no redundant leading zeroes)", digits 10..35 as lowercase
// letters, a leading '-' for negative values; /1: on success ec == errc{} and ptr points one
// past the last character written; "Otherwise, the member ec of the return value has the value
// errc::value_too_large, ptr has the value last". [charconv.from.chars]/1,/4: from_chars
// parses the same strings back; a value not representable in the type gives
// errc::result_out_of_range with ptr past the pattern and value unmodified.
// Every value at a digit-count boundary (b^k - 1, b^k, b^k + 1, and their negatives) of every
// standard integer type in every base 2..36, written into buffers of exactly the needed size
// and one smaller, against a digit-by-digit oracle; at run time and in constant evaluation.
#include <charconv>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <system_error>
#include <type_traits>
#include "check.hpp"

// oracle: digits of v (magnitude) in base b, most significant first; returns the length
template <class U>
constexpr int oracle(U mag, bool neg, int base, char* out) {
  char tmp[160];
  int n = 0;
  do {
    int d = static_cast<int>(mag % static_cast<U>(base));
    tmp[n++] = static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10);
    mag = static_cast<U>(mag / static_cast<U>(base));
  } while (mag != 0);
  int len = 0;
  if (neg) out[len++] = '-';
  while (n > 0) out[len++] = tmp[--n];
  return len;
}

template <class T>
constexpr bool one(T v, int base) {
  using U = std::make_unsigned_t<T>;
  const bool neg = v < 0;
  const U mag = neg ? static_cast<U>(U(0) - static_cast<U>(v)) : static_cast<U>(v);
  char want[160] = {};
  const int len = oracle<U>(mag, neg, base, want);
  char buf[160];
  for (int i = 0; i <= len; ++i) buf[i] = '#';
  // exactly enough room
  auto r = std::to_chars(buf, buf + len, v, base);
  if (r.ec != std::errc{} || r.ptr != buf + len) return false;
  for (int i = 0; i < len; ++i)
    if (buf[i] != want[i]) return false;
  if (buf[len] != '#') return false;
  // one character short
  r = std::to_chars(buf, buf + len - 1, v, base);
  if (r.ec != std::errc::value_too_large || r.ptr != buf + len - 1) return false;
  // plenty of room
  r = std::to_chars(buf, buf + sizeof buf, v, base);
  if (r.ec != std::errc{} || r.ptr != buf + len) return false;
  // back again
  T back = T(0);
  if (v == T(0)) back = T(1);
  auto p = std::from_chars(want, want + len, back, base);
  if (p.ec != std::errc{} || p.ptr != want + len || back != v) return false;
  return true;
}

// one past the extreme: out of range, value unmodified
template <class T>
constexpr bool past_extremes(int base) {
  using U = std::make_unsigned_t<T>;
  char s[160];
  // max + 1 for an unsigned type (and for a signed type the magnitude of min + 1, as "-" form)
  U mx = static_cast<U>(std::numeric_limits<T>::max());
  // compute digits of max + 1 with a carry over the oracle digits of max
  int len = oracle<U>(mx, false, base, s);
  int i = len - 1;
  for (; i >= 0; --i) {
    int d = s[i] <= '9' ? s[i] - '0' : s[i] - 'a' + 10;
    if (d + 1 < base) {
      ++d;
      s[i] = static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10);
      break;
    }
    s[i] = '0';
  }
  if (i < 0) {
    for (int j = len; j > 0; --j) s[j] = s[j - 1];
    s[0] = '1';
    ++len;
  }
  T v = T(7);
  auto r = std::from_chars(s, s + len, v, base);
  if (r.ec != std::errc::result_out_of_range || r.ptr != s + len || v != T(7)) return false;
  if constexpr (std::is_signed_v<T>) {
    // "-" followed by the magnitude of min - 1 = max + 2
    char t[161];
    t[0] = '-';
    for (int j = 0; j < len; ++j) t[j + 1] = s[j];
    // max + 1 is exactly |min|: parses
    T w = T(7);
    r = std::from_chars(t, t + len + 1, w, base);
    if (r.ec != std::errc{} || w != std::numeric_limits<T>::min()) return false;
    // |min| + 1: increment the last digit (|min| is a power of two, never all (base-1)s)
    int d = t[len] <= '9' ? t[len] - '0' : t[len] - 'a' + 10;
    if (d + 1 < base) {
      ++d;
      t[len] = static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10);
      w = T(7);
      r = std::from_chars(t, t + len + 1, w, base);
      if (r.ec != std::errc::result_out_of_range || r.ptr != t + len + 1 || w != T(7)) return false;
    }
  }
  return true;
}

template <class T>
constexpr bool all_boundaries(bool every_base) {
  using U = std::make_unsigned_t<T>;
  constexpr U umax = static_cast<U>(std::numeric_limits<T>::max());
  for (int base = 2; base <= 36; ++base) {
    if (!every_base && base != 2 && base != 7 && base != 10 && base != 16 && base != 36) continue;
    if (!one<T>(T(0), base) || !one<T>(T(1), base)) return false;
    if (!one<T>(std::numeric_limits<T>::max(), base) || !one<T>(std::numeric_limits<T>::min(), base)) return false;
    if (!one<T>(static_cast<T>(std::numeric_limits<T>::max() - 1), base)) return false;
    if constexpr (std::is_signed_v<T>)
      if (!one<T>(static_cast<T>(std::numeric_limits<T>::min() + 1), base) || !one<T>(T(-1), base)) return false;
    U p = 1;  // base^k
    while (true) {
      for (U q : {static_cast<U>(p - 1), p, static_cast<U>(p + 1)}) {
        if (q > umax) continue;
        T t = static_cast<T>(q);
        if (!one<T>(t, base)) return false;
        if constexpr (std::is_signed_v<T>)
          if (!one<T>(static_cast<T>(-t), base)) return false;
      }
      if (p > umax / static_cast<U>(base)) break;
      p = static_cast<U>(p * static_cast<U>(base));
    }
    if (!past_extremes<T>(base)) return false;
  }
  return true;
}

template <class... Ts>
constexpr bool all_types(bool every_base) {
  return (all_boundaries<Ts>(every_base) && ...);
}

// constant evaluation: bases 2, 7, 10, 16, 36 (evaluation limits)
static_assert(all_types<signed char, unsigned char, short, unsigned short>(false));
static_assert(all_types<int>(false));
static_assert(all_types<unsigned>(false));
static_assert(all_types<long long>(false));
static_assert(all_types<unsigned long long>(false));

int main() {
  CHECK((all_types<char, signed char, unsigned char, short, unsigned short, int, unsigned, long, unsigned long,
                   long long, unsigned long long>(true)));
}
