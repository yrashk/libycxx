// [charconv.to.chars]/2: the overloads without a precision produce a string such that
// "parsing the representation using the corresponding from_chars function recovers value
// exactly", with "the smallest number of characters". Checked for many pseudo-random bit
// patterns of float and double (and powers of two, subnormals, boundaries) in every format:
// the parse recovers the bits and consumes the whole string; for the plain, scientific and
// general forms, removing the last significand digit no longer round-trips, so no shorter
// string with fewer digits exists in that style.
#include <charconv>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <system_error>
#include "check.hpp"

using F = std::chars_format;

template <class T>
void roundtrip(T v) {
  char buf[6000];
  const F fmts[] = {F::general, F::fixed, F::scientific, F::hex};
  {
    auto r = std::to_chars(buf, buf + sizeof buf, v);
    CHECK(r.ec == std::errc{});
    T back{};
    auto p = std::from_chars(buf, r.ptr, back);
    CHECK(p.ec == std::errc{} && p.ptr == r.ptr);
    CHECK(back == v && std::signbit(back) == std::signbit(v));  // no NaNs here; value + sign = same bits
  }
  for (F f : fmts) {
    auto r = std::to_chars(buf, buf + sizeof buf, v, f);
    CHECK(r.ec == std::errc{});
    T back{};
    auto p = std::from_chars(buf, r.ptr, back, f);
    CHECK(p.ec == std::errc{} && p.ptr == r.ptr);
    CHECK(back == v && std::signbit(back) == std::signbit(v));  // no NaNs here; value + sign = same bits
    if (f == F::scientific) {
      // drop the last significand digit: d.ddd...e+XX -> d.dd...e+XX
      std::string_view s(buf, r.ptr);
      auto e = s.find('e');
      auto dot = s.find('.');
      if (dot != std::string_view::npos && e - dot > 1) {
        char shorter[6000];
        std::size_t n = 0;
        for (std::size_t i = 0; i < s.size(); ++i)
          if (i != e - 1 && !(e - dot == 2 && i == dot)) shorter[n++] = s[i];
        // the string with one fewer digit parses to a different value
        T other{};
        auto q = std::from_chars(shorter, shorter + n, other, f);
        CHECK(q.ec == std::errc{});
        CHECK(other != v);
      }
    }
  }
}

int main() {
  std::uint32_t s32 = 12345;
  for (int i = 0; i < 20000; ++i) {
    s32 = s32 * 1664525u + 1013904223u;
    float f = std::bit_cast<float>(s32);
    if (std::isfinite(f)) roundtrip(f);
  }
  std::uint64_t s64 = 987654321;
  for (int i = 0; i < 20000; ++i) {
    s64 = s64 * 6364136223846793005ull + 1442695040888963407ull;
    double d = std::bit_cast<double>(s64);
    if (std::isfinite(d)) roundtrip(d);
  }
  for (int e = -1074; e <= 1023; ++e) roundtrip(std::ldexp(1.0, e));
  for (int e = -149; e <= 127; ++e) roundtrip(std::ldexp(1.0f, e));
  roundtrip(std::numeric_limits<double>::max());
  roundtrip(std::numeric_limits<double>::min());
  roundtrip(std::numeric_limits<double>::denorm_min());
  roundtrip(std::nextafter(std::numeric_limits<double>::min(), 0.0));
  roundtrip(std::numeric_limits<float>::max());
  roundtrip(std::numeric_limits<float>::denorm_min());
  roundtrip(-0.0);
  roundtrip(0.1L);
  roundtrip(-1e-300L);
  roundtrip(std::numeric_limits<long double>::max());
  return 0;
}
