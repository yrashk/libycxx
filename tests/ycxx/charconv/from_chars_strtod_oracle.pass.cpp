// [charconv.from.chars]/1: "value is set to the parsed value, after rounding according to
// round_to_nearest ([round.style])"; /6: the pattern is "the expected form of the subject
// sequence in the "C" locale, as described for strtod", for chars_format::general,
// scientific (exponent required) and fixed (no exponent); "if the parsed value is not in the
// range representable by the type of value, value is unmodified and the member ec of the
// return value is equal to errc::result_out_of_range". The C library's strtod / strtof are
// the oracle (correctly rounded): for many pairs of adjacent doubles (floats), the exact
// decimal expansion of their midpoint (exactly representable in long double / double) is
// parsed as is (a tie: to even), extended with a far trailing 1 (just above), truncated to
// fewer digits (at or below), and with its last digit lowered and many 9s appended (just
// below); the same values are parsed in fixed notation when the magnitude allows. The
// midpoint between DBL_MAX and 2^1024 rounds to even, i.e. overflows.
#include <charconv>
#include <bit>
#include <cerrno>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <system_error>
#include "check.hpp"

static unsigned long failures = 0, checked = 0;

template <class T>
static void check_string(const std::string& s, std::chars_format fmt) {
  errno = 0;
  char* end = nullptr;
  T expect;
  if constexpr (std::is_same_v<T, float>) expect = std::strtof(s.c_str(), &end);
  else expect = std::strtod(s.c_str(), &end);
  const bool overflow = std::isinf(expect);
  if (expect == 0 || end != s.c_str() + s.size()) return;  // underflow to zero: not checked
  T v = T(42);
  auto r = std::from_chars(s.data(), s.data() + s.size(), v, fmt);
  ++checked;
  bool ok = overflow ? (r.ec == std::errc::result_out_of_range && v == T(42) && r.ptr == s.data() + s.size())
                     : (r.ec == std::errc{} && r.ptr == s.data() + s.size() && v == expect);
  if (!ok && failures++ < 10)
    dprintf(2, "from_chars(\"%.80s...\" (%zu chars), fmt %d): got %a (ec %d), strtod %a\n", s.c_str(), s.size(),
            static_cast<int>(fmt), static_cast<double>(v), static_cast<int>(r.ec), static_cast<double>(expect));
}

// mid is exactly representable in M; prints its exact expansion and variants.
template <class T, class M>
static void around(M mid) {
  static char b[6000];
  const char* lf = std::is_same_v<M, long double> ? "%.*Le" : "%.*e";
  int n = snprintf(b, sizeof b, lf, 1200, mid);
  CHECK(n > 0 && n < static_cast<int>(sizeof b));
  std::string s(b, n);
  const auto epos = s.find('e');
  std::string mant = s.substr(0, epos), exp = s.substr(epos);
  while (mant.back() == '0') mant.pop_back();
  if (mant.back() == '.') mant.pop_back();
  const std::string exact = mant + exp;
  check_string<T>(exact, std::chars_format::general);
  check_string<T>(exact, std::chars_format::scientific);
  check_string<T>(mant + std::string(300, '0') + "1" + exp, std::chars_format::general);
  for (std::size_t k : {18u, 25u, 40u})
    if (mant.size() > k) check_string<T>(mant.substr(0, k) + exp, std::chars_format::general);
  if (mant.size() > 2 && mant.back() > '0') {
    std::string lower = mant;
    --lower.back();
    check_string<T>(lower + std::string(500, '9') + exp, std::chars_format::general);
  }
  // fixed notation for moderate magnitudes
  const int e10 = std::atoi(exp.c_str() + 1);
  if (e10 > -40 && e10 < 40) {
    const char* ff = std::is_same_v<M, long double> ? "%.*Lf" : "%.*f";
    n = snprintf(b, sizeof b, ff, 1200, mid);
    std::string f(b, n);
    while (f.back() == '0') f.pop_back();
    check_string<T>(f, std::chars_format::fixed);
    check_string<T>(f + std::string(200, '0') + "1", std::chars_format::fixed);
  }
}

static std::uint64_t state = 12345;
static std::uint64_t next() {
  std::uint64_t z = (state += 0x9e3779b97f4a7c15ull);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
  return z ^ (z >> 31);
}

int main() {
  if constexpr (std::numeric_limits<long double>::digits >= 54) {
    auto dmid = [](double lo) {
      const double hi = std::nextafter(lo, std::numeric_limits<double>::infinity());
      long double h = std::isinf(hi) ? std::ldexp(1.0L, 1024) : static_cast<long double>(hi);
      around<double>((static_cast<long double>(lo) + h) / 2);
    };
    for (double d : {1.0, 0.1, 0.3, 1e23, 9007199254740992.0, 5e-324, 2.2250738585072009e-308,
                     2.2250738585072014e-308, DBL_MAX, 123.456, 1e-5, 1e22, 0.5})
      dmid(d);
    for (int i = 0; i < 400; ++i) {
      double d = std::fabs(std::bit_cast<double>(next()));
      if (std::isfinite(d)) dmid(d);
    }
    for (int i = 0; i < 200; ++i) dmid(std::ldexp(static_cast<double>(next() >> 11), static_cast<int>(next() % 100) - 80));
  }
  auto fmid = [](float lo) {
    const float hi = std::nextafter(lo, std::numeric_limits<float>::infinity());
    double h = std::isinf(hi) ? std::ldexp(1.0, 128) : static_cast<double>(hi);
    around<float>((static_cast<double>(lo) + h) / 2);
  };
  for (float f : {1.0f, 0.1f, 1e-45f, FLT_MIN, FLT_MAX, 16777216.0f, 3.0f})
    fmid(f);
  for (int i = 0; i < 400; ++i) {
    float f = std::fabs(std::bit_cast<float>(static_cast<std::uint32_t>(next())));
    if (std::isfinite(f)) fmid(f);
  }
  CHECK(checked > 1000);
  CHECK(failures == 0);
  return 0;
}
