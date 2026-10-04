// [charconv.to.chars]/12-13: to_chars(first, last, value, fmt, precision): "value is converted
// to a string in the style of printf in the "C" locale with the given precision" (%.Pf for
// fixed, %.Pe for scientific, %.Pg for general); /1: "If the member ec of the return value is
// such that the value is equal to the value of a value-initialized errc, the conversion was
// successful and the member ptr is the one-past-the-end pointer of the characters written.
// Otherwise, the member ec has the value errc::value_too_large, the member ptr has the value
// last, and the contents of the range [first, last) are unspecified."
// The C library's snprintf is the oracle: the same printf conversion (correctly rounded from
// the exact binary value) is computed for many doubles (powers, exact decimal ties, subnormals,
// pseudo-random bit patterns), floats (printed through their exact double value) and long
// doubles, at many precisions, and the result must fit exactly in a buffer of its own length
// and fail with value_too_large in one character less. The shortest form ([charconv.to.chars]
// /2-3, no precision) must round-trip through from_chars and fit exactly too.
#include <charconv>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <system_error>
#include <vector>
#include "check.hpp"

static char buf[6000];
static char ref[6000];
static unsigned long failures = 0;

template <class T>
static void one(T v, std::chars_format f, int prec) {
  const char spec = f == std::chars_format::fixed ? 'f' : f == std::chars_format::scientific ? 'e' : 'g';
  int n;
  if constexpr (sizeof(T) == sizeof(long double) && !std::is_same_v<T, double>) {
    const char fmt[] = {'%', '.', '*', 'L', spec, 0};
    n = snprintf(ref, sizeof ref, fmt, prec, v);
  } else {
    const char fmt[] = {'%', '.', '*', spec, 0};
    n = snprintf(ref, sizeof ref, fmt, prec, static_cast<double>(v));
  }
  CHECK(n > 0 && n < static_cast<int>(sizeof ref));
  auto r = std::to_chars(buf, buf + n, v, f, prec);
  if (r.ec != std::errc{} || r.ptr != buf + n || std::memcmp(buf, ref, n) != 0) {
    if (failures++ < 10)
      dprintf(2, "mismatch: %c prec %d: printf \"%s\" to_chars \"%.*s\" (ec %d)\n", spec, prec, ref,
              static_cast<int>(r.ptr - buf), buf, static_cast<int>(r.ec));
    return;
  }
  auto s = std::to_chars(buf, buf + n - 1, v, f, prec);
  if (s.ec != std::errc::value_too_large || s.ptr != buf + n - 1) {
    if (failures++ < 10) dprintf(2, "no value_too_large: %c prec %d \"%s\"\n", spec, prec, ref);
  }
}

template <class T>
static void shortest(T v) {
  auto r = std::to_chars(buf, buf + sizeof buf, v);
  CHECK(r.ec == std::errc{});
  const int n = static_cast<int>(r.ptr - buf);
  T back{};
  auto p = std::from_chars(buf, r.ptr, back);
  if (p.ec != std::errc{} || p.ptr != r.ptr || back != v) {
    if (failures++ < 10) dprintf(2, "shortest does not round-trip: \"%.*s\"\n", n, buf);
  }
  auto s = std::to_chars(buf, buf + n - 1, v);
  if (s.ec != std::errc::value_too_large || s.ptr != buf + n - 1) {
    if (failures++ < 10) dprintf(2, "shortest: no value_too_large at %d\n", n - 1);
  }
}

static const int precs[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 24, 30, 40, 60, 100};

template <class T>
static void all(T v) {
  for (T x : {v, -v}) {
    for (int p : precs) {
      one(x, std::chars_format::fixed, p);
      one(x, std::chars_format::scientific, p);
      one(x, std::chars_format::general, p);
    }
    shortest(x);
  }
}

static std::uint64_t state = 0x9e3779b97f4a7c15ull;
static std::uint64_t next() {  // splitmix64
  std::uint64_t z = (state += 0x9e3779b97f4a7c15ull);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
  return z ^ (z >> 31);
}

int main() {
  std::vector<double> ds = {0.0, 1.0, 0.5, 0.125, 0.375, 2.5, 3.5, 0.05, 0.15, 0.25, 0.35, 1.5, 9.5, 99.5, 999.5,
                            1e-4, 9.9999e-5, 0.0001, 1e15, 1e16, 1e17, 1e21, 1e22, 1e23, 123456789012345678.0,
                            5e-324, 1e-310, 2.2250738585072009e-308, 2.2250738585072014e-308,
                            1.7976931348623157e308, 0.1, 0.2, 0.3, 1.0 / 3, 2.0 / 3, 9.5367431640625e-07,
                            4.35, 0.045, 1.005, 2.675, 1e100, 8.5, 0.000244140625};
  for (int e = -1074; e <= 1023; e += 7) ds.push_back(std::ldexp(1.0, e));
  for (int e = -30; e <= 30; ++e) ds.push_back(std::pow(10.0, e));
  for (int i = 0; i < 400; ++i) {
    double d = std::bit_cast<double>(next());
    if (std::isfinite(d)) ds.push_back(std::fabs(d));
  }
  for (int i = 0; i < 300; ++i) {  // moderate magnitudes, where fixed is interesting
    ds.push_back(std::ldexp(static_cast<double>(next() >> 11), static_cast<int>(next() % 120) - 100));
  }
  for (double d : ds) all(d);

  std::vector<float> fs = {0.0f, 1.0f, 0.5f, 2.5f, 0.1f, 1e-45f, 1.17549435e-38f, 3.40282347e38f, 16777216.0f,
                           1e7f, 9.999999e6f, 1.00000005e-4f, 0.3f, 8.5f};
  for (int i = 0; i < 300; ++i) {
    float f = std::bit_cast<float>(static_cast<std::uint32_t>(next()));
    if (std::isfinite(f)) fs.push_back(std::fabs(f));
  }
  for (float f : fs) all(f);

  if constexpr (std::numeric_limits<long double>::digits == 64) {
    std::vector<long double> ls = {0.0L, 1.0L, 2.5L, 0.1L, 1e-4951L, 1e4932L, 3.6451995318824746025e-4951L,
                                   1.18973149535723176502e4932L};
    for (int i = 0; i < 200; ++i)
      ls.push_back(std::ldexp(static_cast<long double>(next()), static_cast<int>(next() % 200) - 130));
    for (long double l : ls) all(l);
  }
  CHECK(failures == 0);
  return 0;
}
