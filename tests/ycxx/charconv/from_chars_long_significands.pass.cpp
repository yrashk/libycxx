// [charconv.from.chars]/1: "value is set to the parsed value, after rounding according to
// round_to_nearest ([round.style])"; /6.3: the pattern is that of strtod in the "C" locale.
// So the number of significant digits does not matter: decimal strings with 18 to 800
// significant digits (around the 19/20-digit boundary of a 64-bit accumulator in particular),
// long runs of zeros inside the significand, before it (0.000...0ddd) and after it
// (ddd000...0 with and without a decimal point), exact ties between adjacent doubles/floats
// padded with zeros (still ties: to even), and subnormal results. The C library's
// strtod/strtof (correctly rounded) is the oracle; the whole string must be consumed.
#include <charconv>
#include <cerrno>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <system_error>
#include <type_traits>
#include "check.hpp"

static unsigned long failures = 0, checked = 0;

template <class T>
static void check(const std::string& s, std::chars_format fmt = std::chars_format::general) {
  char* end = nullptr;
  T expect;
  if constexpr (std::is_same_v<T, float>) expect = std::strtof(s.c_str(), &end);
  else expect = std::strtod(s.c_str(), &end);
  CHECK(end == s.c_str() + s.size());
  if (expect == 0 || std::isinf(expect)) return;  // range errors are covered elsewhere
  T v = T(42);
  auto r = std::from_chars(s.data(), s.data() + s.size(), v, fmt);
  ++checked;
  if ((r.ec != std::errc{} || r.ptr != s.data() + s.size() || v != expect) && failures++ < 12)
    dprintf(2, "from_chars<%s>(\"%.70s...\" (%zu chars), fmt %d): got %a (ec %d, consumed %td), want %a\n",
            std::is_same_v<T, float> ? "float" : "double", s.c_str(), s.size(), static_cast<int>(fmt),
            static_cast<double>(v), static_cast<int>(r.ec), r.ptr - s.data(), static_cast<double>(expect));
}

static std::uint64_t state = 0x243f6a8885a308d3ull;
static std::uint64_t rnd() {
  std::uint64_t z = (state += 0x9e3779b97f4a7c15ull);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
  return z ^ (z >> 31);
}
static std::string digits(int n, bool nonzero_first = true) {
  std::string s;
  for (int i = 0; i < n; ++i) s += static_cast<char>('0' + rnd() % 10);
  if (nonzero_first && n > 0 && s[0] == '0') s[0] = static_cast<char>('1' + rnd() % 9);
  return s;
}

template <class T>
static void significand_lengths() {
  const int lo_exp = std::is_same_v<T, float> ? -50 : -330;
  const int hi_exp = std::is_same_v<T, float> ? 35 : 300;
  for (int len : {17, 18, 19, 20, 21, 22, 23, 25, 30, 40, 60, 100, 300, 800})
    for (int i = 0; i < 60; ++i) {
      std::string d = digits(len);
      int e = lo_exp + static_cast<int>(rnd() % static_cast<std::uint64_t>(hi_exp - lo_exp));
      // d[0].d[1..]e<e>, and the same digits with a decimal point elsewhere (and leading zeros)
      check<T>(d.substr(0, 1) + "." + d.substr(1) + "e" + std::to_string(e));
      check<T>(d + "e" + std::to_string(e - len + 1), std::chars_format::scientific);
      check<T>("0.0000000000000000000000" + d + "e" + std::to_string(e + 23));
      check<T>("00000000000000000000000000" + d.substr(0, 3) + "." + d.substr(3) + "e" + std::to_string(e - 2));
      if (e > -30 && e < 30) {
        // fixed notation: insert the point by hand
        std::string f;
        if (e >= 0) {
          std::string all = d + std::string(static_cast<std::size_t>(e + 1 > len ? e + 1 - len : 0), '0');
          f = all.substr(0, static_cast<std::size_t>(e) + 1) + "." + all.substr(static_cast<std::size_t>(e) + 1);
        } else {
          f = "0." + std::string(static_cast<std::size_t>(-e - 1), '0') + d;
        }
        check<T>(f, std::chars_format::fixed);
      }
    }
}

// The first 19 digits are fixed and the rest decide rounding: values just around the 19-digit
// truncation.
template <class T>
static void boundary_19_20() {
  for (int i = 0; i < 300; ++i) {
    std::string head = digits(19);
    int e = std::is_same_v<T, float> ? static_cast<int>(rnd() % 60) - 40 : static_cast<int>(rnd() % 600) - 320;
    std::string ex = "e" + std::to_string(e);
    for (const char* tail : {"", "0", "1", "4", "5", "6", "9", "49999999999999999999", "50000000000000000000",
                             "50000000000000000001", "99999999999999999999"})
      check<T>(head.substr(0, 1) + "." + head.substr(1) + tail + ex);
    // 20 digits where the 20th is the first that does not fit in 64 bits
    check<T>("18446744073709551615" + ex);
    check<T>("18446744073709551616" + ex);
    check<T>("18446744073709551617" + ex);
    check<T>("99999999999999999999" + ex);
    check<T>("10000000000000000000" + ex);
    check<T>("9999999999999999999" + ex);
  }
}

// Zero runs inside the significand: d000...000d with runs of many lengths.
template <class T>
static void zero_runs() {
  for (int run : {1, 15, 16, 17, 18, 19, 20, 21, 40, 100, 400, 1000})
    for (int i = 0; i < 25; ++i) {
      std::string a = digits(1 + static_cast<int>(rnd() % 18));
      std::string b = digits(1 + static_cast<int>(rnd() % 5), false);
      int e = std::is_same_v<T, float> ? static_cast<int>(rnd() % 70) - 45 : static_cast<int>(rnd() % 620) - 320;
      std::string z(static_cast<std::size_t>(run), '0');
      check<T>(a.substr(0, 1) + "." + a.substr(1) + z + b + "e" + std::to_string(e));
      check<T>(a.substr(0, 1) + "." + a.substr(1) + z + "e" + std::to_string(e));  // trailing zeros only
      check<T>(a + z + "e" + std::to_string(e - static_cast<int>(a.size()) - run));  // integer with trailing zeros
      check<T>(a + z + "." + z + "e" + std::to_string(e - static_cast<int>(a.size()) - run));
    }
}

// Exact decimal expansion of the midpoint mid (representable in M), padded with zeros.
template <class T, class M>
static void ties(M mid) {
  static char buf[6000];
  int n = std::snprintf(buf, sizeof buf, std::is_same_v<M, long double> ? "%.*Le" : "%.*e", 1200, mid);
  CHECK(n > 0 && n < static_cast<int>(sizeof buf));
  std::string s(buf, static_cast<std::size_t>(n));
  auto ep = s.find('e');
  std::string mant = s.substr(0, ep), ex = s.substr(ep);
  while (mant.back() == '0') mant.pop_back();
  CHECK(mant.size() < 1100);  // the expansion is exact (not cut by the precision)
  check<T>(mant + ex);
  check<T>(mant + std::string(400, '0') + ex);
  std::string up = mant + std::string(400, '0') + "1" + ex;
  check<T>(up);
  std::string m2 = mant;
  if (m2.back() > '0') {
    --m2.back();
    check<T>(m2 + std::string(30, '9') + ex);
  }
}

int main() {
  significand_lengths<double>();
  significand_lengths<float>();
  boundary_19_20<double>();
  boundary_19_20<float>();
  zero_runs<double>();
  zero_runs<float>();
  if constexpr (std::numeric_limits<long double>::digits >= 64) {
    // ties between subnormal doubles (k + 1/2) * 2^-1074, and around the normal boundary
    for (std::uint64_t k : {1ull, 2ull, 3ull, 4ull, 7ull, 1000ull, 123456789ull, (1ull << 52) - 1, (1ull << 52),
                            (1ull << 52) + 1, (1ull << 53) - 1})
      ties<double>(std::ldexp(static_cast<long double>(k) + 0.5L, -1074));
    for (int i = 0; i < 40; ++i)
      ties<double>(std::ldexp(static_cast<long double>(rnd() % (1ull << 52)) + 0.5L, -1074));
    // ties among normal doubles at random binades
    for (int i = 0; i < 60; ++i) {
      std::uint64_t m = (1ull << 52) | (rnd() & ((1ull << 52) - 1));
      ties<double>(std::ldexp(static_cast<long double>(m) + 0.5L, static_cast<int>(rnd() % 2000) - 1074));
    }
  }
  for (std::uint32_t k : {1u, 2u, 3u, 77u, (1u << 23) - 1, 1u << 23, (1u << 24) - 1})
    ties<float>(std::ldexp(static_cast<double>(k) + 0.5, -149));
  for (int i = 0; i < 60; ++i) {
    std::uint32_t m = (1u << 23) | static_cast<std::uint32_t>(rnd() & ((1u << 23) - 1));
    ties<float>(std::ldexp(static_cast<double>(m) + 0.5, static_cast<int>(rnd() % 250) - 149));
  }
  // subnormal results from long strings
  for (int i = 0; i < 200; ++i) {
    std::string d = digits(20 + static_cast<int>(rnd() % 60));
    check<double>(d.substr(0, 1) + "." + d.substr(1) + "e-" + std::to_string(308 + rnd() % 16));
    check<float>(d.substr(0, 1) + "." + d.substr(1) + "e-" + std::to_string(38 + rnd() % 8));
  }
  CHECK(checked > 20000);
  CHECK(failures == 0);
}
