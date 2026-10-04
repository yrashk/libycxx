// [bit.pow.two]: has_single_bit, bit_ceil, bit_floor, bit_width at edge values.
#include <bit>
#include <cstdint>
#include <limits>
#include "check.hpp"

template <class T>
constexpr bool test_type() {
  constexpr int N = std::numeric_limits<T>::digits;
  constexpr T max = std::numeric_limits<T>::max();
  constexpr T top = T(T(1) << (N - 1));
  // has_single_bit: true iff x is an integral power of two.
  if (std::has_single_bit(T(0)) || !std::has_single_bit(T(1)) || !std::has_single_bit(T(2))) return false;
  if (std::has_single_bit(T(3)) || std::has_single_bit(max) || !std::has_single_bit(top)) return false;
  if (std::has_single_bit(T(top + 1)) || std::has_single_bit(T(top - 1))) return false;
  // bit_ceil: smallest power of 2 >= x. bit_ceil(0) == 1 (2^0 >= 0).
  if (std::bit_ceil(T(0)) != T(1) || std::bit_ceil(T(1)) != T(1)) return false;
  if (std::bit_ceil(T(2)) != T(2) || std::bit_ceil(T(3)) != T(4) || std::bit_ceil(T(5)) != T(8)) return false;
  if (std::bit_ceil(top) != top) return false;
  if (std::bit_ceil(T(top - 1)) != top) return false;
  if (std::bit_ceil(T((top >> 1) + 1)) != top) return false;
  // bit_floor: 0 for 0, else largest power of 2 <= x.
  if (std::bit_floor(T(0)) != T(0) || std::bit_floor(T(1)) != T(1)) return false;
  if (std::bit_floor(T(3)) != T(2) || std::bit_floor(max) != top || std::bit_floor(top) != top) return false;
  if (std::bit_floor(T(top - 1)) != T(top >> 1)) return false;
  // bit_width: 0 for 0, otherwise 1 + floor(log2 x).
  if (std::bit_width(T(0)) != 0 || std::bit_width(T(1)) != 1 || std::bit_width(T(2)) != 2) return false;
  if (std::bit_width(T(3)) != 2 || std::bit_width(T(4)) != 3) return false;
  if (std::bit_width(max) != N || std::bit_width(top) != N || std::bit_width(T(top - 1)) != N - 1) return false;
  for (int i = 0; i < N; ++i) {
    T p = T(T(1) << i);
    if (!std::has_single_bit(p) || std::bit_ceil(p) != p || std::bit_floor(p) != p) return false;
    if (std::bit_width(p) != i + 1) return false;
    if (i >= 1 && std::bit_floor(T(p | T(p - 1))) != p) return false;
    if (i + 1 < N && std::bit_ceil(T(p + 1)) != T(p << 1) && p != 1) return false;
  }
  return true;
}

constexpr bool test() {
  return test_type<unsigned char>() && test_type<unsigned short>() && test_type<unsigned>() &&
         test_type<unsigned long>() && test_type<unsigned long long>() &&
         std::bit_ceil(std::uint8_t(127)) == 128 && std::bit_ceil(std::uint8_t(128)) == 128 &&
         std::bit_ceil(std::uint16_t(0x7FFF)) == 0x8000 && std::bit_floor(std::uint8_t(0xFF)) == 0x80 &&
         std::bit_width(std::uint8_t(0xFF)) == 8 && std::bit_width(0x10000u) == 17;
}
static_assert(test());

int main() {
  CHECK(test());
  volatile unsigned v = 1000;
  CHECK(std::bit_ceil(unsigned(v)) == 1024u);
  CHECK(std::bit_floor(unsigned(v)) == 512u);
  CHECK(std::bit_width(unsigned(v)) == 10);
  CHECK(!std::has_single_bit(unsigned(v)));
  return 0;
}
