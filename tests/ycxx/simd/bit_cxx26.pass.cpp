// [simd.bit]/3-4 bit_reverse, /13-16 shl and shr (a vector of counts or one count), /17-24 rotl,
// rotr and bit_repeat with a vector of counts or one int, /25-26 the counting functions'
// rebind_t<make_signed_t<T>, V> result, /27-30 bit_compress and bit_expand (a vector of masks or
// one mask): each element is the scalar <bit> function applied to the corresponding elements,
// checked against <bit> for many values, in constant evaluation. The Constraints: bit_reverse,
// bit_ceil, bit_repeat, bit_compress and the counting functions need unsigned elements; the
// two-vector forms of shl/shr and rotl/rotr/bit_repeat need equal sizes and equal element sizes.
#include <simd>
#include <bit>
#include <cstdint>
#include <type_traits>
#include "check.hpp"

namespace simd = std::simd;
using U8 = simd::vec<std::uint8_t, 8>;
using U32 = simd::vec<std::uint32_t, 4>;
using I32 = simd::vec<std::int32_t, 4>;
using U64x4 = simd::vec<std::uint64_t, 4>;

template <class V>
concept rev_ok = requires(V v) { simd::bit_reverse(v); };
template <class A, class B>
concept shl_ok = requires(A a, B b) { simd::shl(a, b); };
template <class A, class B>
concept rotl_ok = requires(A a, B b) { simd::rotl(a, b); };
template <class V>
concept compress_ok = requires(V v) { simd::bit_compress(v, v); };
static_assert(rev_ok<U32> && !rev_ok<I32>);
static_assert(shl_ok<U32, I32> && shl_ok<I32, U32> && shl_ok<U32, int> && !shl_ok<U32, U64x4>);
static_assert(!shl_ok<U32, simd::vec<std::int32_t, 8>>);
static_assert(rotl_ok<U32, I32> && rotl_ok<U32, int> && !rotl_ok<U32, simd::vec<std::int16_t, 4>>);
static_assert(compress_ok<U32> && !compress_ok<I32>);
static_assert(std::is_same_v<decltype(simd::popcount(U32())), simd::rebind_t<std::int32_t, U32>>);
static_assert(std::is_same_v<decltype(simd::countl_zero(U8())), simd::rebind_t<std::int8_t, U8>>);
static_assert(std::is_same_v<decltype(simd::bit_width(U64x4())), simd::rebind_t<std::int64_t, U64x4>>);

template <class T, int N, class F>
constexpr simd::vec<T, N> make(F f) {
  return simd::vec<T, N>([&](int i) { return static_cast<T>(f(i)); });
}

constexpr bool test() {
  const std::uint32_t seeds[] = {0u, 1u, 0x80000000u, 0xdeadbeefu, 0x0f0f00ffu, 0xffffffffu, 0x12345678u, 7u};
  for (int k = 0; k < 8; k += 4) {
    U32 v = make<std::uint32_t, 4>([&](int i) { return seeds[k + i]; });
    U32 m = make<std::uint32_t, 4>([&](int i) { return seeds[(k + i + 3) % 8] ^ 0x00ff00ffu; });
    I32 counts = make<std::int32_t, 4>([&](int i) { return (k + i) * 9 - 13; }); // negative and >= 32 too
    U32 ucounts = make<std::uint32_t, 4>([&](int i) { return (k + i) * 11; });
    U32 lens = make<std::uint32_t, 4>([&](int i) { return 1 + (k + i) * 5; });
    auto rev = simd::bit_reverse(v);
    auto sl = simd::shl(v, counts);
    auto sr = simd::shr(v, ucounts);
    auto sl1 = simd::shl(v, 5);
    auto sr1 = simd::shr(v, -3);
    auto rl = simd::rotl(v, counts);
    auto rr = simd::rotr(v, ucounts);
    auto rp = simd::bit_repeat(v, lens);
    auto rp1 = simd::bit_repeat(v, 3);
    auto cp = simd::bit_compress(v, m);
    auto ex = simd::bit_expand(v, m);
    auto cp1 = simd::bit_compress(v, 0xf0f0f0f0u);
    auto ex1 = simd::bit_expand(v, 0x0000ffffu);
    auto pc = simd::popcount(v);
    auto cl = simd::countl_one(v);
    for (int i = 0; i < 4; ++i) {
      std::uint32_t x = v[i];
      if (rev[i] != std::bit_reverse(x)) return false;
      if (sl[i] != std::shl(x, counts[i]) || sr[i] != std::shr(x, ucounts[i])) return false;
      if (sl1[i] != std::shl(x, 5) || sr1[i] != std::shr(x, -3)) return false;
      if (rl[i] != std::rotl(x, static_cast<int>(counts[i])) || rr[i] != std::rotr(x, static_cast<int>(ucounts[i]))) return false;
      if (rp[i] != std::bit_repeat(x, static_cast<int>(lens[i])) || rp1[i] != std::bit_repeat(x, 3)) return false;
      if (cp[i] != std::bit_compress(x, std::uint32_t(m[i])) || ex[i] != std::bit_expand(x, std::uint32_t(m[i]))) return false;
      if (cp1[i] != std::bit_compress(x, 0xf0f0f0f0u) || ex1[i] != std::bit_expand(x, 0x0000ffffu)) return false;
      if (pc[i] != std::popcount(x) || cl[i] != std::countl_one(x)) return false;
    }
  }
  // Signed element types for shl/shr.
  I32 s = make<std::int32_t, 4>([](int i) { return -5 + i * 1000; });
  auto ss = simd::shr(s, I32(2));
  for (int i = 0; i < 4; ++i)
    if (ss[i] != std::shr(std::int32_t(s[i]), 2)) return false;
  // Narrow elements.
  U8 b = make<std::uint8_t, 8>([](int i) { return i * 37 + 1; });
  auto rb = simd::bit_reverse(b);
  for (int i = 0; i < 8; ++i)
    if (rb[i] != std::bit_reverse(std::uint8_t(b[i]))) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
