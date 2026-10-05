// [simd.mask.ctor]: basic_mask(bool) broadcasts; the generator constructor calls gen(integral_
// constant<simd-size-type, i>()) once per i in increasing order; bitset<size()> and unsigned
// integer constructors (/8: the first M elements from the bits of val, the rest false); the
// converting constructor copies the elements.
// [simd.mask.unary]: ! is element-wise; + - ~ return vec<integer-from<Bytes>, size()> holding
// +k[i], -k[i], ~k[i] (bool promoted to int and converted).
// [simd.mask.conv]: conversion to basic_vec<U, A> of equal width, explicit iff sizeof(U) !=
// Bytes, elements static_cast<U>(k[i]); to_bitset(), to_ullong().
// [simd.mask.binary], [simd.mask.cassign], [simd.mask.comparison]: element-wise && || & | ^ and
// == != < <= > >= returning masks.
// [simd.mask.cond]/[simd.alg]/10: select(mask, mask, mask), select(mask, bool, bool) give masks;
// select(mask, T, T) with sizeof(T) == Bytes gives vec<T, size()>.
// COUNTERPART: libcxx:experimental/simd/simd.mask.class/.*
#include <simd>
#include <bitset>
#include <cstdint>
#include <type_traits>
#include "check.hpp"

namespace simd = std::simd;
using V = simd::vec<int, 6>;
using M = V::mask_type;

constexpr bool bits(const M& m, unsigned pattern) {
  for (int i = 0; i < 6; ++i)
    if (m[i] != bool(pattern >> i & 1)) return false;
  return true;
}

constexpr bool test() {
  CHECK(bits(M(true), 0b111111) && bits(M(false), 0) && bits(M(), 0));
  M g([](auto i) { return i % 3 == 0; });
  CHECK(bits(g, 0b001001));
  M fromb = std::bitset<6>("100110");
  CHECK(bits(fromb, 0b100110));
  M fromu(0b101011u);
  CHECK(bits(fromu, 0b101011));
  M fromlarge(0xFFFFFFC1u);  // only the first 6 bits are used
  CHECK(bits(fromlarge, 0b000001));
  M fromsmall(static_cast<std::uint8_t>(0x3F));
  CHECK(bits(fromsmall, 0b111111));
  simd::mask<char, 6> other(g);
  CHECK(other[0] && !other[1] && other[3]);
  M back(other);
  CHECK(bits(back, 0b001001));

  CHECK(fromu.to_bitset() == std::bitset<6>(0b101011));
  CHECK(fromu.to_ullong() == 0b101011ull);
  static_assert(std::is_same_v<decltype(fromu.to_bitset()), std::bitset<6>>);

  CHECK(bits(!g, 0b110110));
  auto plus = +g;
  static_assert(std::is_same_v<decltype(plus), simd::vec<int, 6>>);  // integer-from<4> is int
  CHECK(plus[0] == 1 && plus[1] == 0);
  auto minus = -g;
  CHECK(minus[0] == -1 && minus[1] == 0);
  auto tilde = ~g;
  CHECK(tilde[0] == -2 && tilde[1] == -1);
  static_assert(std::is_same_v<decltype(+simd::mask<short, 3>()), simd::vec<short, 3>>);
  static_assert(std::is_same_v<decltype(-simd::mask<double, 2>()), simd::vec<long long, 2>> ||
                sizeof(long) == 8);  // integer-from<8>: some 64-bit signed type
  static_assert(sizeof(decltype(-simd::mask<double, 2>())::value_type) == 8 &&
                std::is_signed_v<decltype(-simd::mask<double, 2>())::value_type>);
  static_assert(std::is_same_v<decltype(+simd::mask<signed char, 4>())::value_type, signed char> ||
                std::is_same_v<decltype(+simd::mask<signed char, 4>())::value_type, char>);

  // conversion to vec
  V asint = g;  // sizeof(int) == 4 == Bytes: implicit
  CHECK(asint[0] == 1 && asint[1] == 0 && asint[3] == 1);
  simd::vec<float, 6> asfloat = g;
  CHECK(asfloat[0] == 1.0f && asfloat[2] == 0.0f);
  static_assert(!std::is_convertible_v<M, simd::vec<short, 6>> && std::is_constructible_v<simd::vec<short, 6>, M>);
  static_assert(std::is_convertible_v<M, simd::vec<unsigned, 6>>);
  static_assert(!std::is_constructible_v<simd::vec<int, 5>, M>);

  // binary
  M a(0b001111u), b(0b010101u);
  CHECK(bits(a && b, 0b000101) && bits(a || b, 0b011111));
  CHECK(bits(a & b, 0b000101) && bits(a | b, 0b011111) && bits(a ^ b, 0b011010));
  M c = a;
  c &= b;
  CHECK(bits(c, 0b000101));
  c |= M(0b100000u);
  CHECK(bits(c, 0b100101));
  c ^= M(true);
  CHECK(bits(c, 0b011010));
  CHECK(&(c ^= a) == &c);
  CHECK(bits(a == b, 0b100101) && bits(a != b, 0b011010));
  CHECK(bits(a < b, 0b010000) && bits(a > b, 0b001010));  // false < true
  CHECK(bits(a <= b, 0b110101) && bits(a >= b, 0b101111));

  // select
  auto sm = simd::select(g, a, b);
  static_assert(std::is_same_v<decltype(sm), M>);
  CHECK(bits(sm, 0b011101));
  auto sb = simd::select(g, true, false);
  static_assert(std::is_same_v<decltype(sb), M>);
  CHECK(bits(sb, 0b001001));
  auto sv = simd::select(g, 5, 7);
  static_assert(std::is_same_v<decltype(sv), simd::vec<int, 6>>);
  CHECK(sv[0] == 5 && sv[1] == 7);
  auto sf = simd::select(g, 1.5f, 2.5f);
  static_assert(std::is_same_v<decltype(sf), simd::vec<float, 6>>);
  CHECK(sf[3] == 1.5f && sf[4] == 2.5f);

  // iteration
  int n = 0;
  for (bool x : g) n += x;
  CHECK(n == 2);
  return true;
}
static_assert(test());

int main() {
  test();
  int calls = 0, order[6] = {};
  M m([&](int i) {
    order[calls++] = i;
    return i > 3;
  });
  CHECK(calls == 6 && order[0] == 0 && order[5] == 5 && m[4] && !m[3]);
  simd::mask<char, 64> wide(true);
  CHECK(wide.to_ullong() == ~0ull && simd::reduce_count(wide) == 64);
  return 0;
}
