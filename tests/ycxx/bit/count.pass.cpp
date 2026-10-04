// [bit.count]: countl_zero, countl_one, countr_zero, countr_one, popcount on every
// unsigned integer type. Notes 1-4: countl_zero(0) == countr_zero(0) == N and
// countl_one(max) == countr_one(max) == N, where N = numeric_limits<T>::digits.
#include <bit>
#include <cstdint>
#include <limits>
#include "check.hpp"

template <class T>
constexpr bool test_type() {
  constexpr int N = std::numeric_limits<T>::digits;
  constexpr T max = std::numeric_limits<T>::max();
  constexpr T top = T(T(1) << (N - 1));
  if (std::countl_zero(T(0)) != N || std::countr_zero(T(0)) != N) return false;
  if (std::countl_one(max) != N || std::countr_one(max) != N) return false;
  if (std::countl_one(T(0)) != 0 || std::countr_one(T(0)) != 0) return false;
  if (std::countl_zero(max) != 0 || std::countr_zero(max) != 0) return false;
  if (std::popcount(T(0)) != 0 || std::popcount(max) != N) return false;
  // Narrow types must not count promoted bits.
  if (std::countl_zero(T(1)) != N - 1) return false;
  if (std::countl_zero(top) != 0) return false;
  if (std::countr_zero(top) != N - 1) return false;
  if (std::countl_one(T(max - 1)) != N - 1) return false;  // 111...10
  if (std::countr_one(T(max >> 1)) != N - 1) return false; // 011...11
  if (std::countl_one(T(max >> 1)) != 0) return false;
  if (std::countr_one(T(max - 1)) != 0) return false;
  if (std::countl_one(top) != 1) return false;
  // Each single-bit value.
  for (int i = 0; i < N; ++i) {
    T v = T(T(1) << i);
    if (std::countr_zero(v) != i) return false;
    if (std::countl_zero(v) != N - 1 - i) return false;
    if (std::popcount(v) != 1) return false;
    T low = T(v - 1); // i low ones
    if (std::countr_one(low) != i) return false;
    if (std::popcount(low) != i) return false;
    if (std::countl_one(T(~low)) != N - i) return false;
  }
  if (std::popcount(T(0x55)) != 4) return false;
  return true;
}

constexpr bool test() {
  return test_type<unsigned char>() && test_type<unsigned short>() && test_type<unsigned>() &&
         test_type<unsigned long>() && test_type<unsigned long long>() &&
         test_type<std::uint8_t>() && test_type<std::uint16_t>() && test_type<std::uint32_t>() &&
         test_type<std::uint64_t>() && test_type<std::uintmax_t>() && test_type<std::size_t>() &&
         std::countl_zero(std::uint8_t(0x10)) == 3 && std::countl_one(std::uint8_t(0xF0)) == 4 &&
         std::countr_zero(std::uint16_t(0x0100)) == 8 && std::countr_one(0x0000FFFFu) == 16 &&
         std::popcount(0xDEADBEEFu) == 24 && std::popcount(0x8000000000000001ull) == 2;
}
static_assert(test());

int main() {
  CHECK(test());
  volatile unsigned char z = 0;
  CHECK(std::countl_zero((unsigned char)z) == 8);
  CHECK(std::countl_one((unsigned short)(unsigned short)~0u) == 16);
  return 0;
}
