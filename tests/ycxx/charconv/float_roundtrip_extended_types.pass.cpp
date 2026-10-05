// Round trips of to_chars / from_chars for the floating-point types other than float and
// double: every finite bit pattern of float16_t and bfloat16_t, and pseudo-random values of
// long double, float128_t, float32_t and float64_t, in every chars_format.
// [charconv.syn]/1: overloads exist for every cv-unqualified floating-point type, including the
// extended ones. [charconv.to.chars]/2: without a precision, the string "consists of the
// smallest number of characters such that ... parsing the representation using the
// corresponding from_chars function recovers value exactly" -- checked as: the parse with the
// same format consumes the whole string and gives back the same bits (signed zeros included),
// and for the scientific form one significant digit fewer (correctly rounded, by the precision
// overload) no longer round-trips (skipped where the value is a power of two, whose rounding
// interval is asymmetric). /1: with a buffer one character too short the result is
// {last, errc::value_too_large}. [charconv.from.chars]/7: hex is parsed without "0x".
#include <charconv>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdfloat>
#include <string_view>
#include <system_error>
#include "check.hpp"

constexpr std::chars_format formats[] = {std::chars_format::general, std::chars_format::fixed,
                                         std::chars_format::scientific, std::chars_format::hex};
char buf[20000];

template <class T>
bool same_bits(T a, T b) {
  return std::memcmp(&a, &b, sizeof(T) < 16 ? sizeof(T) : (std::numeric_limits<T>::digits == 64 ? 10 : 16)) == 0;
}

template <class T>
bool round_trips(std::string_view s, T v, bool use_fmt, std::chars_format f) {
  T back = T(12345);
  auto r = use_fmt ? std::from_chars(s.data(), s.data() + s.size(), back, f)
                   : std::from_chars(s.data(), s.data() + s.size(), back);
  return r.ec == std::errc{} && r.ptr == s.data() + s.size() && same_bits(back, v);
}

template <class T>
void check_value(T v, bool power_of_two, bool small_buffer_check) {
  // no format
  {
    auto r = std::to_chars(buf, buf + sizeof buf, v);
    CHECK(r.ec == std::errc{});
    std::string_view s(buf, static_cast<std::size_t>(r.ptr - buf));
    CHECK(round_trips(s, v, false, std::chars_format::general));
    if (small_buffer_check) {
      auto t = std::to_chars(buf, buf + s.size() - 1, v);
      CHECK(t.ec == std::errc::value_too_large && t.ptr == buf + s.size() - 1);
    }
  }
  for (auto f : formats) {
    auto r = std::to_chars(buf, buf + sizeof buf, v, f);
    CHECK(r.ec == std::errc{});
    std::string_view s(buf, static_cast<std::size_t>(r.ptr - buf));
    CHECK(!s.starts_with("0x") && !s.starts_with("-0x"));
    CHECK(round_trips(s, v, true, f));
    if (small_buffer_check) {
      auto t = std::to_chars(buf, buf + s.size() - 1, v, f);
      CHECK(t.ec == std::errc::value_too_large && t.ptr == buf + s.size() - 1);
    }
    if (f == std::chars_format::scientific && !power_of_two && v != T(0)) {
      // significant digits of the shortest scientific form
      std::size_t digits = 0;
      for (char ch : s.substr(0, s.find('e')))
        if (ch >= '0' && ch <= '9') ++digits;
      if (digits >= 2) {
        char shorter[256];
        auto q = std::to_chars(shorter, shorter + sizeof shorter, v, f, static_cast<int>(digits) - 2);
        CHECK(q.ec == std::errc{});
        CHECK(!round_trips(std::string_view(shorter, static_cast<std::size_t>(q.ptr - shorter)), v, true, f));
      }
    }
  }
}

template <class T, class U>
void exhaustive16() {
  for (std::uint32_t bits = 0; bits <= 0xffffu; ++bits) {
    T v = std::bit_cast<T>(static_cast<U>(bits));
    if (!(v == v) || v == std::numeric_limits<T>::infinity() || v == -std::numeric_limits<T>::infinity()) continue;
    const unsigned mant_bits = static_cast<unsigned>(std::numeric_limits<T>::digits) - 1;
    bool p2 = (bits & ((1u << mant_bits) - 1)) == 0;
    check_value(v, p2, bits % 7 == 0);
  }
}

std::uint64_t s64 = 0x9e3779b97f4a7c15ull;
std::uint64_t r64() {
  s64 ^= s64 << 13;
  s64 ^= s64 >> 7;
  s64 ^= s64 << 17;
  return s64;
}

// m * 2^e computed exactly (except for the final rounding into subnormals or overflow avoided by
// the exponent range), for m with at most digits<T> significant bits.
template <class T>
T scaled(T m, int e) {
  T p = 1, b = e < 0 ? T(0.5) : T(2);
  for (unsigned k = static_cast<unsigned>(e < 0 ? -e : e); k; k >>= 1, b *= b)
    if (k & 1) p *= b;
  return m * p;
}

template <class T>
void random_values(int count) {
  constexpr int digits = std::numeric_limits<T>::digits;
  constexpr int max_e = std::numeric_limits<T>::max_exponent;
  constexpr int min_e = std::numeric_limits<T>::min_exponent - digits;
  for (int i = 0; i < count; ++i) {
    // a random significand of exactly `digits` bits (top bit set), or fewer
    T m = 0;
    int bits_left = (i % 5 == 0) ? 1 + static_cast<int>(r64() % 8) : digits;
    T unit = 1;
    for (int b = 0; b < bits_left; ++b) {
      if (b == bits_left - 1 || (r64() & 1)) m += unit;
      unit *= 2;
    }
    // m < 2^bits_left; choose e so that m * 2^e is finite and at least the smallest subnormal
    int lo = min_e;
    int hi = max_e - bits_left;
    int e = lo + static_cast<int>(r64() % static_cast<std::uint64_t>(hi - lo));
    // half of the cases near 1, where the fixed form is short
    if (i % 2) e = -bits_left + static_cast<int>(r64() % 40) - 20;
    T v = scaled(m, e);
    if (r64() & 1) v = -v;
    check_value(v, bits_left == 1, i % 3 == 0);
  }
  check_value(std::numeric_limits<T>::max(), false, true);
  check_value(std::numeric_limits<T>::lowest(), false, true);
  check_value(std::numeric_limits<T>::min(), true, true);
  check_value(std::numeric_limits<T>::denorm_min(), true, true);
  check_value(T(0), true, true);
  check_value(-T(0), true, true);
}

int main() {
#ifdef __STDCPP_FLOAT16_T__
  exhaustive16<std::float16_t, std::uint16_t>();
  random_values<std::float16_t>(500);
#endif
#ifdef __STDCPP_BFLOAT16_T__
  exhaustive16<std::bfloat16_t, std::uint16_t>();
#endif
#ifdef __STDCPP_FLOAT32_T__
  random_values<std::float32_t>(3000);
#endif
#ifdef __STDCPP_FLOAT64_T__
  random_values<std::float64_t>(3000);
#endif
#ifdef __STDCPP_FLOAT128_T__
  random_values<std::float128_t>(1500);
#endif
  random_values<long double>(1500);
}
