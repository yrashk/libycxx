// [bitset.cons]/1: constexpr bitset() noexcept: "Initializes all bits in *this to zero."
// [bitset.cons]/2: constexpr bitset(unsigned long long val) noexcept: "Initializes the first M
// bit positions to the corresponding bit values in val. M is the smaller of N and the number
// of bits in the value representation of unsigned long long. If M < N, the remaining bit
// positions are initialized to zero."
#include <bitset>
#include <climits>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_nothrow_default_constructible_v<std::bitset<8>>);
static_assert(std::is_nothrow_constructible_v<std::bitset<8>, unsigned long long>);
static_assert(std::is_convertible_v<unsigned long long, std::bitset<8>>);  // not explicit
static_assert(std::is_trivially_copyable_v<std::bitset<100>>);
constexpr std::size_t ull_bits = sizeof(unsigned long long) * CHAR_BIT;

constexpr bool test() {
  std::bitset<10> d;
  if (d.any() || d.count() != 0) return false;

  std::bitset<4> t(0xFFull);  // truncated to N bits
  if (t.count() != 4 || t.to_ulong() != 0xF) return false;

  std::bitset<8> e(0b1010'0101ull);
  if (!e[0] || e[1] || !e[2] || e[5] == false || !e[7]) return false;
  if (e.to_ullong() != 0xA5) return false;

  std::bitset<200> big(~0ull);  // M = bits of unsigned long long, rest zero
  if (big.count() != ull_bits) return false;
  for (std::size_t i = 0; i < ull_bits; ++i)
    if (!big.test(i)) return false;
  for (std::size_t i = ull_bits; i < 200; ++i)
    if (big.test(i)) return false;

  std::bitset<0> z(123);
  if (z.size() != 0 || z.any()) return false;

  std::bitset<64> conv = 5;  // implicit conversion from integer
  if (conv.to_ullong() != 5) return false;
  return true;
}
static_assert(test());

constinit std::bitset<70> g(0x8000'0000'0000'0001ull);

int main() {
  CHECK(test());
  CHECK(g.count() == 2 && g.test(0) && g.test(63) && !g.test(64));
  return 0;
}
