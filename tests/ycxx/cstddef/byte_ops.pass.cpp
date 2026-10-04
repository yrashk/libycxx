// [cstddef.syn]: enum class byte : unsigned char {}; [support.types.byteops]:
//   operator<<(byte b, IntType shift): "return static_cast<byte>(static_cast<unsigned int>(b)
//     << shift);" (likewise >>); operator<<= / >>=: "return b = b << shift;"
//   |, &, ^: "static_cast<byte>(static_cast<unsigned int>(l) OP static_cast<unsigned int>(r))";
//   |=, &=, ^=: "return l = l OP r;"; ~: "static_cast<byte>(~static_cast<unsigned int>(b))";
//   to_integer<IntType>(b): "return static_cast<IntType>(b);".
// All constexpr and noexcept.
#include <cstddef>
#include <type_traits>
#include "check.hpp"

using std::byte;
static_assert(std::is_enum_v<byte> && !std::is_convertible_v<byte, int>);
static_assert(std::is_same_v<std::underlying_type_t<byte>, unsigned char>);
static_assert(sizeof(byte) == 1);

constexpr byte b1{0x81};
static_assert(noexcept(b1 << 1) && noexcept(b1 >> 1) && noexcept(b1 | b1) && noexcept(b1 & b1));
static_assert(noexcept(b1 ^ b1) && noexcept(~b1) && noexcept(std::to_integer<int>(b1)));
static_assert(std::is_same_v<decltype(b1 << 1), byte> && std::is_same_v<decltype(b1 >> 1ull), byte>);
static_assert(std::is_same_v<decltype(~b1), byte> && std::is_same_v<decltype(b1 | b1), byte>);
static_assert(std::is_same_v<decltype(std::to_integer<long>(b1)), long>);

constexpr bool test() {
  byte b{0x81};
  if (std::to_integer<int>(b << 1) != 0x02) return false;   // high bit discarded
  if (std::to_integer<int>(b << 7) != 0x80) return false;
  if (std::to_integer<int>(b << 8) != 0) return false;      // unsigned int shift, then truncated
  if (std::to_integer<int>(b >> 7) != 0x01) return false;
  if (std::to_integer<int>(b >> 1) != 0x40) return false;   // no sign extension
  if (std::to_integer<int>(b << 1ull) != 0x02 || std::to_integer<int>(b >> short(1)) != 0x40) return false;
  if (std::to_integer<int>(b << true) != 0x02) return false;  // bool is an integral type
  if (std::to_integer<int>(b << char(4)) != 0x10) return false;
  byte c{0x0F};
  if (std::to_integer<int>(b | c) != 0x8F || std::to_integer<int>(b & c) != 0x01) return false;
  if (std::to_integer<int>(b ^ c) != 0x8E || std::to_integer<int>(~c) != 0xF0) return false;
  if (~byte{0} != byte{0xFF}) return false;
  byte d{0x01};
  byte& r1 = (d <<= 3);
  if (&r1 != &d || d != byte{0x08}) return false;
  byte& r2 = (d >>= 2);
  if (&r2 != &d || d != byte{0x02}) return false;
  byte& r3 = (d |= byte{0x10});
  if (&r3 != &d || d != byte{0x12}) return false;
  byte& r4 = (d &= byte{0x30});
  if (&r4 != &d || d != byte{0x10}) return false;
  byte& r5 = (d ^= byte{0x11});
  if (&r5 != &d || d != byte{0x01}) return false;
  d = byte{0xC0};
  d <<= 1;
  if (d != byte{0x80}) return false;
  // to_integer: static_cast to the requested type
  if (std::to_integer<signed char>(byte{0xFF}) != static_cast<signed char>(0xFF)) return false;
  if (std::to_integer<unsigned long long>(byte{0xFF}) != 0xFFull) return false;
  if (std::to_integer<bool>(byte{0x02}) != true || std::to_integer<bool>(byte{0}) != false) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
