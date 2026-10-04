// [bit.rotate]: rotl/rotr with r = s % N; r == 0 -> x; r > 0 -> (x << r) | (x >> (N - r));
// r < 0 -> rotr(x, -r) (resp. rotl(x, -r)). Covers negative and out-of-range counts and
// every unsigned integer type (no integer-promotion artefacts for narrow types).
#include <bit>
#include <climits>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include "check.hpp"

template <class T>
constexpr bool test_type() {
  constexpr int N = std::numeric_limits<T>::digits;
  constexpr T top = T(T(1) << (N - 1));
  const T x = T(top | T(0x5)); // 1000...0101
  // Rotating by 1.
  if (std::rotl(T(1), 1) != T(2)) return false;
  if (std::rotl(top, 1) != T(1)) return false;
  if (std::rotr(T(1), 1) != top) return false;
  if (std::rotr(top, 1) != T(top >> 1)) return false;
  // r == 0 cases: 0, N, -N, multiples of N.
  for (int s : {0, N, -N, 2 * N, -3 * N}) {
    if (std::rotl(x, s) != x) return false;
    if (std::rotr(x, s) != x) return false;
  }
  // Negative counts rotate the other way.
  for (int s = -2 * N; s <= 2 * N; ++s) {
    if (std::rotl(x, s) != std::rotr(x, -s)) return false;
    if (std::rotr(std::rotl(x, s), s) != x) return false;
    if (std::rotl(x, s) != std::rotl(x, s % N)) return false;
  }
  // Extreme counts: INT_MIN % N == 0 (N is a power of 2); INT_MAX % N == N - 1.
  if (std::rotl(x, INT_MIN) != x) return false;
  if (std::rotr(x, INT_MIN) != x) return false;
  if (std::rotl(x, INT_MAX) != std::rotr(x, 1)) return false;
  if (std::rotr(x, INT_MAX) != std::rotl(x, 1)) return false;
  if (std::rotl(x, INT_MIN + 1) != std::rotr(x, N - 1)) return false;
  // All-ones and zero are fixed points.
  T ones = std::numeric_limits<T>::max();
  if (std::rotl(ones, 3) != ones || std::rotr(T(0), 5) != T(0)) return false;
  return true;
}

constexpr bool test_values() {
  if (std::rotl(std::uint8_t(0x81), 1) != 0x03) return false;
  if (std::rotr(std::uint8_t(0x81), 1) != 0xC0) return false;
  if (std::rotl(std::uint8_t(0x12), 4) != 0x21) return false;
  if (std::rotl(std::uint8_t(0x12), -9) != 0x09) return false;   // -9 % 8 == -1 -> rotr 1
  if (std::rotl(std::uint16_t(0x8001), 1) != 0x0003) return false;
  if (std::rotr(std::uint16_t(0x0003), 17) != 0x8001) return false;
  if (std::rotl(0x12345678u, 8) != 0x34567812u) return false;
  if (std::rotr(0x12345678u, 8) != 0x78123456u) return false;
  if (std::rotl(0x12345678u, -8) != 0x78123456u) return false;
  if (std::rotl(std::uint64_t(0x0123456789ABCDEF), 4) != std::uint64_t(0x123456789ABCDEF0)) return false;
  if (std::rotr(std::uint64_t(0x0123456789ABCDEF), 68) != std::uint64_t(0xF0123456789ABCDE)) return false;
  return true;
}

constexpr bool test() {
  return test_type<unsigned char>() && test_type<unsigned short>() && test_type<unsigned>() &&
         test_type<unsigned long>() && test_type<unsigned long long>() && test_values();
}
static_assert(test());

int main() {
  CHECK(test());
  volatile int s = -1;
  CHECK(std::rotl(std::uint8_t(1), int(s)) == 0x80);
  CHECK(std::rotr(1u, int(s)) == 2u);
  return 0;
}
