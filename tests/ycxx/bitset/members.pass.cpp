// [bitset.members]: &=, |=, ^=, <<=, >>=, <<, >>, set, reset, flip, ~, count, size, ==, test,
// all, any, none. Shifts: "If I < pos, the new value is zero; If I >= pos, the new value is the
// previous value of the bit at position I - pos" and ">>=: If pos >= N - I, the new value is
// zero". all() is count() == size(), any() is count() != 0, none() is count() == 0.
#include <bitset>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

template <std::size_t N>
constexpr bool test_n() {
  using B = std::bitset<N>;
  B x;
  if (x.size() != N || !x.none() || x.any() || x.all() != (N == 0)) return false;
  B& r1 = x.set();
  if (&r1 != &x || x.count() != N || !x.all() || (N && x.none())) return false;
  B& r2 = x.reset();
  if (&r2 != &x || x.count() != 0) return false;
  if (N == 0) return true;
  x.set(N / 2, true);
  if (!x.test(N / 2) || x.count() != 1) return false;
  x.set(N / 2, false);
  if (x.any()) return false;
  x.set(0);
  x.set(N - 1);
  if (!x.test(0) || !x.test(N - 1) || (N > 2 && x.test(N / 2))) return false;
  B& r3 = x.flip(0);
  if (&r3 != &x || x.test(0)) return false;
  B& r4 = x.reset(N - 1);
  if (&r4 != &x || x.any()) return false;
  B y = ~x;
  if (!y.all() || x.any()) return false;
  B& r5 = x.flip();
  if (&r5 != &x || x != y || !(x == y)) return false;
  // shifts
  B one(1);
  B shifted = one << (N - 1);
  if (!shifted.test(N - 1) || shifted.count() != 1) return false;
  if ((shifted >> (N - 1)) != one) return false;
  if ((one << N).any() || (y >> N).any() || (y << (N + 1000)).any()) return false;
  if ((y >> 1).count() != N - 1 || (y >> 1).test(N - 1)) return false;
  if ((y << 1).count() != N - 1 || (y << 1).test(0)) return false;
  B s = y;
  B& r6 = (s <<= 1);
  if (&r6 != &s || s != (y << 1)) return false;
  B& r7 = (s >>= 1);
  if (&r7 != &s || s.test(N - 1) || s.count() != N - 1) return false;
  return true;
}

constexpr bool test_bitwise() {
  std::bitset<8> a(0b1100'1010), b(0b1010'0110);
  std::bitset<8> c = a;
  if (&(c &= b) != &c || c.to_ulong() != 0b1000'0010) return false;
  c = a;
  if (&(c |= b) != &c || c.to_ulong() != 0b1110'1110) return false;
  c = a;
  if (&(c ^= b) != &c || c.to_ulong() != 0b0110'1100) return false;
  if ((a & b).to_ulong() != 0b1000'0010) return false;
  if ((a | b).to_ulong() != 0b1110'1110) return false;
  if ((a ^ b).to_ulong() != 0b0110'1100) return false;
  if ((~a).to_ulong() != 0b0011'0101) return false;
  if (a.count() != 4) return false;
  std::bitset<8> s(0b1001'0110);
  if ((s << 3).to_ulong() != 0b1011'0000 || (s >> 3).to_ulong() != 0b0001'0010) return false;
  return true;
}

static_assert(std::is_same_v<decltype(std::bitset<4>() & std::bitset<4>()), std::bitset<4>>);
static_assert(std::is_same_v<decltype(std::bitset<4>() == std::bitset<4>()), bool>);
static_assert(std::is_same_v<decltype(std::bitset<4>().count()), std::size_t>);
static_assert(test_n<0>());
static_assert(test_n<1>());
static_assert(test_n<7>());
static_assert(test_n<64>());
static_assert(test_n<65>());
static_assert(test_n<130>());
static_assert(test_bitwise());

int main() {
  CHECK(test_n<0>());
  CHECK(test_n<1>());
  CHECK(test_n<33>());
  CHECK(test_n<64>());
  CHECK(test_n<200>());
  CHECK(test_bitwise());
  return 0;
}
