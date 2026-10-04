// [bit.permute]: bit_reverse, bit_repeat, bit_compress, bit_expand.
// Expected values come from the formulas 22.1-22.4 (reference implementations below are
// literal transcriptions of those sums) and from Examples 1-3 and Note 1.
#include <bit>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include "check.hpp"

template <class T> constexpr int N = std::numeric_limits<T>::digits;
template <class T> constexpr bool bit(T a, int n) { return (a >> n) & 1; }
// sigma(a, n): number of one-bits among the lowest n bits of a.
template <class T> constexpr int sigma(T a, int n) {
  int c = 0;
  for (int k = 0; k < n; ++k) c += bit(a, k);
  return c;
}
template <class T> constexpr T ref_reverse(T x) {
  T r = 0;
  for (int n = 0; n < N<T>; ++n) if (bit(x, n)) r |= T(T(1) << (N<T> - n - 1));
  return r;
}
template <class T> constexpr T ref_repeat(T x, int l) {
  T r = 0;
  for (int n = 0; n < N<T>; ++n) if (bit(x, n % l)) r |= T(T(1) << n);
  return r;
}
template <class T> constexpr T ref_compress(T x, T m) {
  T r = 0;
  for (int n = 0; n < N<T>; ++n) if (bit(m, n) && bit(x, n)) r |= T(T(1) << sigma(m, n));
  return r;
}
template <class T> constexpr T ref_expand(T x, T m) {
  T r = 0;
  for (int n = 0; n < N<T>; ++n) if (bit(m, n) && bit(x, sigma(m, n))) r |= T(T(1) << n);
  return r;
}

constexpr bool test_examples() {
  // Example 1: bit_repeat(uint32_t{0xc}, 4) == 0xcccccccc.
  if (std::bit_repeat(std::uint32_t{0xc}, 4) != 0xccccccccu) return false;
  // Example 2/3 over all 16 values of 0bABCD with mask 0b0101.
  for (unsigned v = 0; v < 16; ++v) {
    unsigned A = (v >> 3) & 1, B = (v >> 2) & 1, C = (v >> 1) & 1, D = v & 1;
    (void)A;
    if (std::bit_compress(v, 0b0101u) != ((B << 1) | D)) return false;      // 0b00BD
    if (std::bit_expand(v, 0b0101u) != ((C << 2) | D)) return false;        // 0b0C0D
  }
  return true;
}

constexpr bool test_values() {
  if (std::bit_reverse(std::uint8_t(0x01)) != 0x80) return false;
  if (std::bit_reverse(std::uint8_t(0x16)) != 0x68) return false;
  if (std::bit_reverse(std::uint16_t(0x1234)) != 0x2C48) return false;
  if (std::bit_reverse(0x12345678u) != 0x1E6A2C48u) return false;
  if (std::bit_reverse(std::uint64_t(0x0123456789ABCDEF)) != std::uint64_t(0xF7B3D591E6A2C480)) return false;
  if (std::bit_reverse(std::uint64_t(1)) != std::uint64_t(1) << 63) return false;
  if (std::bit_repeat(std::uint8_t(0b101), 3) != 0x6D) return false;
  if (std::bit_repeat(std::uint8_t(0xF2), 2) != 0xAA) return false;  // bits above l ignored
  if (std::bit_repeat(std::uint16_t(0x5), 3) != 0xDB6D) return false;
  if (std::bit_repeat(std::uint64_t(0xABCD), 16) != std::uint64_t(0xABCDABCDABCDABCD)) return false;
  if (std::bit_repeat(std::uint8_t(1), 1) != 0xFF) return false;
  if (std::bit_repeat(std::uint8_t(0xFE), 1) != 0x00) return false;
  if (std::bit_repeat(0xDEADBEEFu, 32) != 0xDEADBEEFu) return false;   // l >= N: identity
  if (std::bit_repeat(0xDEADBEEFu, 33) != 0xDEADBEEFu) return false;
  if (std::bit_repeat(0xDEADBEEFu, 0x7FFFFFFF) != 0xDEADBEEFu) return false;
  if (std::bit_compress(0x12345678u, 0xFF00FF00u) != 0x1256u) return false;
  if (std::bit_expand(0x1256u, 0xFF00FF00u) != 0x12005600u) return false;
  if (std::bit_compress(std::uint8_t(0xB6), std::uint8_t(0xF0)) != 0x0B) return false;
  if (std::bit_compress(0xDEADBEEFu, 0x0F0F0F0Fu) != 0xEDEFu) return false;
  if (std::bit_expand(0xFFFFu, 0xAAAAAAAAu) != 0xAAAAAAAAu) return false;
  if (std::bit_compress(std::uint64_t(0x0123456789ABCDEF), std::uint64_t(0xAAAAAAAAAAAAAAAA)) != 0x505AFAF) return false;
  if (std::bit_expand(std::uint16_t(0x12), std::uint16_t(0x0F0F)) != 0x0102) return false;
  if (std::bit_compress(~0ull, 0x8000000000000001ull) != 3) return false;
  if (std::bit_expand(3ull, 0x8000000000000001ull) != 0x8000000000000001ull) return false;
  if (std::bit_expand(2ull, 0x8000000000000001ull) != 0x8000000000000000ull) return false;
  return true;
}

template <class T>
constexpr bool test_identities(T x, T m) {
  constexpr T ones = std::numeric_limits<T>::max();
  int pm = std::popcount(m);
  T low = pm == N<T> ? ones : T((T(1) << pm) - 1);
  T c = std::bit_compress(x, m);
  T e = std::bit_expand(x, m);
  if (c != ref_compress(x, m) || e != ref_expand(x, m)) return false;
  if (std::bit_reverse(x) != ref_reverse(x)) return false;
  if (std::bit_reverse(std::bit_reverse(x)) != x) return false;               // Note 1
  if (std::popcount(std::bit_reverse(x)) != std::popcount(x)) return false;
  if (std::countl_zero(std::bit_reverse(x)) != std::countr_zero(x)) return false;
  if (std::bit_expand(c, m) != T(x & m)) return false;          // expand o compress == x & m
  if (std::bit_compress(e, m) != T(x & low)) return false;      // compress o expand keeps low bits
  if (T(c & ~low) != 0) return false;                           // compress fits in popcount(m) bits
  if (T(e & ~m) != 0) return false;                             // expand stays inside the mask
  if (std::popcount(c) != std::popcount(T(x & m))) return false;
  if (std::popcount(c) > pm || std::popcount(e) > pm) return false;
  if (std::bit_compress(x, ones) != x || std::bit_expand(x, ones) != x) return false;
  if (std::bit_compress(x, T(0)) != 0 || std::bit_expand(x, T(0)) != 0) return false;
  for (int l : {1, 2, 3, 5, N<T> - 1, N<T>, N<T> + 1})
    if (std::bit_repeat(x, l) != ref_repeat(x, l)) return false;
  return true;
}

template <class T>
constexpr bool test_type(bool full) {
  constexpr T samples[] = {T(T(0x1234567890ABCDEFull)), std::numeric_limits<T>::max(), T(0x81),
                           T(0), T(1), T(0x5A), T(0xF0), T(T(0xFEDCBA0987654321ull)),
                           T(T(1) << (N<T> - 1))};
  const int n = full ? 9 : 3; // keep constant evaluation within the compilers' step limits
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      if (!test_identities(samples[i], samples[j])) return false;
  return true;
}

constexpr bool test(bool full = false) {
  return test_examples() && test_values() && test_type<unsigned char>(full) &&
         test_type<unsigned short>(full) && test_type<unsigned>(full) &&
         test_type<unsigned long>(full) && test_type<unsigned long long>(full);
}
static_assert(test());

int main() {
  CHECK(test(true));
  // Exhaustive check against the formulas for 8-bit values at run time.
  for (unsigned x = 0; x < 256; ++x)
    for (unsigned m = 0; m < 256; ++m)
      CHECK(test_identities(std::uint8_t(x), std::uint8_t(m)));
  return 0;
}
