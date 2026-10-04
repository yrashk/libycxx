// [bit.byteswap]: the bytes of the object representation in reverse order;
// [bit.cast]: each bit of the value representation of the result equals the corresponding
// bit of the object representation of from.
#include <bit>
#include <cstdint>
#include <cstring>
#include <limits>
#include "check.hpp"

constexpr bool test_byteswap() {
  if (std::byteswap(std::uint16_t(0x1234)) != 0x3412) return false;
  if (std::byteswap(std::uint32_t(0x12345678)) != 0x78563412u) return false;
  if (std::byteswap(std::uint64_t(0x0102030405060708)) != std::uint64_t(0x0807060504030201)) return false;
  if (std::byteswap(std::uint8_t(0xAB)) != 0xAB) return false;
  if (std::byteswap(std::int8_t(-5)) != -5) return false;
  if (std::byteswap(char('x')) != 'x') return false;
  if (std::byteswap(std::int16_t(0x00FF)) != std::int16_t(-256)) return false;   // 0xFF00
  if (std::byteswap(std::int32_t(-2)) != std::int32_t(0xFEFFFFFF)) return false;
  if (std::byteswap(char16_t(0x1234)) != char16_t(0x3412)) return false;
  if (std::byteswap(char32_t(0x11223344)) != char32_t(0x44332211)) return false;
  if (std::byteswap(std::byteswap(0xDEADBEEFCAFEF00Dull)) != 0xDEADBEEFCAFEF00Dull) return false;
  return true;
}

struct Pair { std::uint16_t a, b; };

constexpr bool test_bit_cast() {
  if (std::bit_cast<std::uint32_t>(1.0f) != 0x3F800000u) return false;
  if (std::bit_cast<std::uint32_t>(-0.0f) != 0x80000000u) return false;
  if (std::bit_cast<double>(std::uint64_t(0x4000000000000000)) != 2.0) return false;
  if (std::bit_cast<std::uint64_t>(std::numeric_limits<double>::infinity()) != 0x7FF0000000000000ull) return false;
  if (std::bit_cast<std::int32_t>(0xFFFFFFFFu) != -1) return false;
  // Round trip through an aggregate.
  Pair p = std::bit_cast<Pair>(std::bit_cast<std::uint32_t>(Pair{0x1111, 0x2222}));
  if (p.a != 0x1111 || p.b != 0x2222) return false;
  return true;
}

static_assert(test_byteswap());
static_assert(test_bit_cast());

int main() {
  CHECK(test_byteswap());
  CHECK(test_bit_cast());
  // byteswap reverses the object representation regardless of endianness.
  volatile std::uint32_t v = 0x11223344u;
  std::uint32_t in = v, out = std::byteswap(in);
  unsigned char a[4], b[4];
  std::memcpy(a, &in, 4);
  std::memcpy(b, &out, 4);
  for (int i = 0; i < 4; ++i) CHECK(a[i] == b[3 - i]);
  long long ll = -123456789012345LL;
  long long sw = std::byteswap(ll);
  unsigned char c[sizeof ll], d[sizeof ll];
  std::memcpy(c, &ll, sizeof ll);
  std::memcpy(d, &sw, sizeof ll);
  for (unsigned i = 0; i < sizeof ll; ++i) CHECK(c[i] == d[sizeof ll - 1 - i]);
  // bit_cast copies the object representation.
  float f = 3.5f;
  std::uint32_t u = std::bit_cast<std::uint32_t>(f), m;
  std::memcpy(&m, &f, 4);
  CHECK(u == m);
  return 0;
}
