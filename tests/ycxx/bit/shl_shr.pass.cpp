// [bit.shift] std::shl / std::shr: "Returns: x×2^s rounded towards negative infinity",
// with the result first computed in a hypothetical integer type of sufficient range and
// then converted to T as if by static_cast<T>(r) ([bit.shift]/1).
#include <bit>
#include <climits>
#include <cstdint>
#include <limits>
#include <type_traits>
#include "check.hpp"

using std::shl;
using std::shr;

constexpr bool test_unsigned() {
  // Ordinary left shifts; bits shifted out are lost by the conversion to T.
  if (shl(std::uint8_t(1), 7) != 128) return false;
  if (shl(std::uint8_t(1), 8) != 0) return false;           // 256 -> uint8_t is 0
  if (shl(std::uint8_t(0xFF), 4) != 0xF0) return false;     // 0xFF0 -> 0xF0
  if (shl(std::uint16_t(0x8001), 1) != 0x0002) return false;
  if (shl(1u, 31) != 0x80000000u) return false;
  if (shl(1u, 32) != 0u) return false;                       // any shift amount allowed
  if (shl(1u, 1000) != 0u) return false;
  if (shl(0xFFFFFFFFu, INT_MAX) != 0u) return false;
  if (shl(std::uint64_t(1), 63) != (std::uint64_t(1) << 63)) return false;
  if (shl(std::uint64_t(1), 64) != 0) return false;
  // Negative amounts shift right (rounding towards -inf == truncation for x >= 0).
  if (shl(std::uint8_t(0xFF), -4) != 0x0F) return false;
  if (shl(1u, -1) != 0u) return false;
  if (shl(0x80000000u, -31) != 1u) return false;
  if (shl(0x80000000u, -32) != 0u) return false;
  if (shl(0xFFFFFFFFu, INT_MIN) != 0u) return false;
  // shr
  if (shr(0x80000000u, 31) != 1u) return false;
  if (shr(0x80000000u, 32) != 0u) return false;
  if (shr(0xFFFFFFFFu, 1000) != 0u) return false;
  if (shr(std::uint64_t(-1), 63) != 1) return false;
  if (shr(std::uint64_t(-1), 64) != 0) return false;
  if (shr(std::uint8_t(3), 1) != 1) return false;
  if (shr(1u, -3) != 8u) return false;                       // negative amount shifts left
  if (shr(1u, -32) != 0u) return false;
  if (shr(std::uint8_t(0x81), -1) != 0x02) return false;
  if (shr(1u, INT_MIN) != 0u) return false;                  // 2^(2^31) mod 2^32 == 0
  // Zero amount is the identity.
  if (shl(0xDEADBEEFu, 0) != 0xDEADBEEFu) return false;
  if (shr(0xDEADBEEFu, 0) != 0xDEADBEEFu) return false;
  return true;
}

constexpr bool test_signed() {
  // Left shifts of negative values are well-defined: x*2^s then wrapped to T.
  if (shl(-1, 1) != -2) return false;
  if (shl(-3, 2) != -12) return false;
  if (shl(1, 31) != INT_MIN) return false;                   // 2^31 -> INT_MIN
  if (shl(1, 32) != 0) return false;                         // 2^32 -> 0
  if (shl(-1, 31) != INT_MIN) return false;
  if (shl(-1, 32) != 0) return false;
  if (shl(INT_MAX, 1) != -2) return false;                   // 2^32 - 2 -> -2
  if (shl(std::int8_t(0x40), 1) != std::int8_t(-128)) return false;
  if (shl(std::int8_t(3), 6) != std::int8_t(-64)) return false;   // 192 -> -64
  if (shl(std::int8_t(-128), 1) != 0) return false;               // -256 -> 0
  // Rounding towards negative infinity.
  if (shl(5, -1) != 2) return false;
  if (shl(-5, -1) != -3) return false;                       // floor(-2.5)
  if (shl(-1, -1) != -1) return false;                       // floor(-0.5)
  if (shl(-1, -1000) != -1) return false;
  if (shl(INT_MIN, -31) != -1) return false;
  if (shl(INT_MIN, -32) != -1) return false;
  if (shl(INT_MAX, -1000) != 0) return false;
  if (shr(-1, 1) != -1) return false;
  if (shr(-3, 1) != -2) return false;                        // floor(-1.5)
  if (shr(-4, 1) != -2) return false;
  if (shr(3, 1) != 1) return false;
  if (shr(-7, 2) != -2) return false;                        // floor(-1.75)
  if (shr(INT_MIN, 31) != -1) return false;
  if (shr(INT_MIN, 32) != -1) return false;
  if (shr(INT_MIN, 100000) != -1) return false;
  if (shr(INT_MAX, 30) != 1) return false;
  if (shr(INT_MAX, 31) != 0) return false;
  if (shr(-1, -1) != -2) return false;                       // negative amount: left shift
  if (shr(std::int8_t(-1), -7) != std::int8_t(-128)) return false;
  if (shr(std::int64_t(-1), 63) != -1) return false;
  if (shr(LLONG_MIN, 63) != -1) return false;
  if (shr(LLONG_MIN, 64) != -1) return false;
  return true;
}

constexpr bool test_shift_types() {
  // S may be any signed or unsigned integer type, of any width.
  if (shl(1u, (unsigned char)3) != 8u) return false;
  if (shl(1u, (signed char)-1) != 0u) return false;
  if (shl(1u, (short)4) != 16u) return false;
  if (shl(1u, 3ull) != 8u) return false;
  if (shl(1u, ULLONG_MAX) != 0u) return false;
  if (shr(1u, ULLONG_MAX) != 0u) return false;
  if (shr(-1, ULLONG_MAX) != -1) return false;
  if (shr(-8, 2ull) != -2) return false;
  if (shl(1, LLONG_MIN) != 0) return false;
  if (shl(-1, LLONG_MIN) != -1) return false;
  if (shr(-1, LLONG_MAX) != -1) return false;
  if (shr(5, LLONG_MIN) != 0) return false;
  if (shl(std::uint64_t(1), std::int8_t(40)) != (std::uint64_t(1) << 40)) return false;
  return true;
}

// Return type is T, independent of S, and the functions are noexcept.
static_assert(std::is_same_v<decltype(shl(std::uint8_t(1), 1ll)), std::uint8_t>);
static_assert(std::is_same_v<decltype(shr(std::int16_t(1), 1u)), std::int16_t>);
static_assert(std::is_same_v<decltype(shl(1ull, (signed char)1)), unsigned long long>);
static_assert(noexcept(shl(1, 1)));
static_assert(noexcept(shr(1u, -1)));

static_assert(test_unsigned());
static_assert(test_signed());
static_assert(test_shift_types());

int main() {
  CHECK(test_unsigned());
  CHECK(test_signed());
  CHECK(test_shift_types());
  // Run-time values that the optimizer cannot fold.
  volatile int big = 1000, neg = -1000, one = 1;
  CHECK(shl(1u, int(big)) == 0u);
  CHECK(shr(-1, int(big)) == -1);
  CHECK(shl(-1, int(neg)) == -1);
  CHECK(shr(1u, int(neg)) == 0u);
  CHECK(shr(-3, int(one)) == -2);
  return 0;
}
