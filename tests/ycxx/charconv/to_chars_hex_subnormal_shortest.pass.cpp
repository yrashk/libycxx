// [charconv.to.chars]/3, /6: to_chars(first, last, value, chars_format::hex) without a precision
// gives the representation with the fewest characters that round-trips. For a subnormal value
// that is the one normalized to a leading 1: 1p-1074, not 0.0000000000001p-1022. With a
// precision, libycxx keeps the C library's leading 0 for subnormal values.
#include <bit>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>
#include <system_error>
#if __has_include(<stdfloat>)
#  include <stdfloat>
#endif
#include "check.hpp"

template <class F>
bool hex_is(F v, std::string_view want, int precision = -1) {
  char buf[64];
  const auto r = precision < 0 ? std::to_chars(buf, buf + sizeof buf, v, std::chars_format::hex)
                               : std::to_chars(buf, buf + sizeof buf, v, std::chars_format::hex, precision);
  if (r.ec != std::errc())
    return false;
  const std::string_view got(buf, static_cast<std::size_t>(r.ptr - buf));
  if (got != want)
    dprintf(2, "got %.*s, want %.*s\n", int(got.size()), got.data(), int(want.size()), want.data());
  return got == want;
}

// Every subnormal value with significand bits m (a sample of them) round-trips, and its shortest
// form is no longer than the form with a leading 0 and the same digits after it.
template <class F, class U>
void sweep(int frac_bits) {
  for (U m = 1; m < (U(1) << frac_bits); m = m * 3 + 1) {
    const F v = std::bit_cast<F>(m);
    char buf[64];
    const auto r = std::to_chars(buf, buf + sizeof buf, v, std::chars_format::hex);
    CHECK(r.ec == std::errc() && buf[0] == '1');
    F back{};
    const auto b = std::from_chars(buf, r.ptr, back, std::chars_format::hex);
    CHECK(b.ec == std::errc() && b.ptr == r.ptr && std::bit_cast<U>(back) == m);
  }
}

int main() {
  CHECK(hex_is(std::numeric_limits<double>::denorm_min(), "1p-1074"));
  CHECK(hex_is(-std::numeric_limits<double>::denorm_min(), "-1p-1074"));
  CHECK(hex_is(std::bit_cast<double>(std::uint64_t(0x0008000000000000)), "1p-1023"));
  CHECK(hex_is(std::bit_cast<double>(std::uint64_t(0x000fffffffffffff)), "1.ffffffffffffep-1023"));
  CHECK(hex_is(std::bit_cast<double>(std::uint64_t(0x0000000000000003)), "1.8p-1073"));
  CHECK(hex_is(std::numeric_limits<float>::denorm_min(), "1p-149"));
  CHECK(hex_is(std::bit_cast<float>(std::uint32_t(0x00400000)), "1p-127"));
  // Normal numbers and zero as before.
  CHECK(hex_is(std::numeric_limits<double>::min(), "1p-1022"));
  CHECK(hex_is(0.0, "0p+0"));
  // With a precision: the C library's form.
  CHECK(hex_is(std::numeric_limits<double>::denorm_min(), "0.0000000000001p-1022", 13));
  CHECK(hex_is(std::numeric_limits<float>::denorm_min(), "0.000002p-126", 6));
#if defined(__STDCPP_FLOAT16_T__)
  CHECK(hex_is(std::bit_cast<std::float16_t>(std::uint16_t(0x01eb)), "1.ebp-16"));
#endif
  sweep<double, std::uint64_t>(52);
  sweep<float, std::uint32_t>(23);
}
