// [charconv.to.chars]/1: if the result does not fit, "the member ec has the value
// errc::value_too_large, the member ptr has the value last, and the contents of the range
// [first, last) are unspecified"; on success ptr is one past the last character written,
// so a buffer of exactly the needed size succeeds. Applies to every floating-point
// overload (plain, fmt, fmt + precision).
#include <charconv>
#include <limits>
#include <string_view>
#include <system_error>
#include "check.hpp"

using F = std::chars_format;

template <class Conv>
void check_sizes(Conv conv) {
  char full[512];
  auto r = conv(full, full + sizeof full);
  CHECK(r.ec == std::errc{});
  const long n = r.ptr - full;
  CHECK(n > 0);
  char small[512];
  for (long len = 0; len < n; ++len) {
    auto s = conv(small, small + len);
    CHECK(s.ec == std::errc::value_too_large);
    CHECK(s.ptr == small + len);
  }
  auto e = conv(small, small + n);
  CHECK(e.ec == std::errc{} && e.ptr == small + n);
  CHECK(std::string_view(small, n) == std::string_view(full, n));
}

int main() {
  const double ds[] = {0.0, -1.5, 0.1, 1e300, -5e-324, 123456.789, std::numeric_limits<double>::infinity()};
  for (double d : ds) {
    check_sizes([d](char* f, char* l) { return std::to_chars(f, l, d); });
    for (F fmt : {F::fixed, F::scientific, F::general, F::hex}) {
      check_sizes([d, fmt](char* f, char* l) { return std::to_chars(f, l, d, fmt); });
      check_sizes([d, fmt](char* f, char* l) { return std::to_chars(f, l, d, fmt, 6); });
    }
  }
  check_sizes([](char* f, char* l) { return std::to_chars(f, l, 3.25f); });
  check_sizes([](char* f, char* l) { return std::to_chars(f, l, -2.0L, F::fixed, 3); });
  check_sizes([](char* f, char* l) { return std::to_chars(f, l, 1e-30, F::fixed, 40); });
  return 0;
}
