// Element-wise arithmetic on narrow integer vectors of a size that is not a power of two, and
// partial loads from short and empty ranges.
// [simd.binary]/[simd.unary]: the operators apply the operator element-wise; for unsigned
// char / signed char elements that is the usual arithmetic conversions followed by the
// conversion back to the element type ([conv.integral]/3: modulo 2^N), so 250 + 250 is 244,
// -(-128) is -128, (-128) + (-128) is 0. Shifts (in range) and / % likewise.
// [simd.loadstore]: partial_load(r) loads the first ranges::size(r) elements and value-
// initializes the others (size 0 included); partial_load(first, n) the same for [first, n).
// [simd.mask.reductions]: reduce_count, reduce_min_index, reduce_max_index.
// [simd.comparison]: comparisons element-wise (a NaN compares unequal to itself).
#include <cmath>
#include <limits>
#include <simd>
#include <vector>
#include "check.hpp"

namespace simd = std::simd;

int main() {
  using V = simd::vec<unsigned char, 33>;
  const V a([](int i) { return static_cast<unsigned char>(250 + i); });  // 250..255, 0..26
  const V sum = a + a, prod = a * a, neg = -a, inv = ~a, shr = a >> 3, shl = a << 3;
  const V quo = a / V(static_cast<unsigned char>(7)), rem = a % V(static_cast<unsigned char>(7));
  for (int i = 0; i < 33; ++i) {
    const unsigned x = static_cast<unsigned char>(250 + i);
    CHECK(sum[i] == static_cast<unsigned char>(x + x));
    CHECK(prod[i] == static_cast<unsigned char>(x * x));
    CHECK(neg[i] == static_cast<unsigned char>(0u - x));
    CHECK(inv[i] == static_cast<unsigned char>(~x));
    CHECK(shr[i] == (x >> 3) && shl[i] == static_cast<unsigned char>(x << 3));
    CHECK(quo[i] == x / 7 && rem[i] == x % 7);
  }
  CHECK(sum[0] == 244 && prod[0] == 36 && neg[7] == 255);

  using S = simd::vec<signed char, 5>;
  const S s([](int i) { return static_cast<signed char>(i == 0 ? -128 : i * 30); });  // -128 30 60 90 120
  const S ss = s + s, sn = -s, sm = s * s, sh = s >> 1;
  const int exp_sum[5] = {0, 60, 120, -76, -16}, exp_neg[5] = {-128, -30, -60, -90, -120},
            exp_mul[5] = {0, -124, 16, -92, 64}, exp_shr[5] = {-64, 15, 30, 45, 60};
  for (int i = 0; i < 5; ++i)
    CHECK(ss[i] == exp_sum[i] && sn[i] == exp_neg[i] && sm[i] == exp_mul[i] && sh[i] == exp_shr[i]);

  using I4 = simd::vec<int, 4>;
  const std::vector<int> empty, three{1, 2, 3};
  const I4 p0 = simd::partial_load<I4>(empty);
  CHECK(p0[0] == 0 && p0[1] == 0 && p0[2] == 0 && p0[3] == 0);
  const I4 p3 = simd::partial_load<I4>(three);
  CHECK(p3[0] == 1 && p3[1] == 2 && p3[2] == 3 && p3[3] == 0);
  const I4 p2 = simd::partial_load<I4>(three.begin(), 2);
  CHECK(p2[0] == 1 && p2[1] == 2 && p2[2] == 0 && p2[3] == 0);
  const I4 pz = simd::partial_load<I4>(three.begin(), 0);
  CHECK(simd::none_of(pz != I4(0)));

  const simd::vec<float, 4> f([](int i) { return i == 1 ? std::numeric_limits<float>::quiet_NaN() : float(i); });
  const auto eq = f == f;
  CHECK(eq[0] && !eq[1] && eq[2] && eq[3] && (f != f)[1] && !(f < f)[1]);

  using I7 = simd::vec<int, 7>;
  const auto m = I7([](int i) { return i; }) > 3;
  CHECK(simd::reduce_count(m) == 3 && simd::reduce_min_index(m) == 4 && simd::reduce_max_index(m) == 6);
  CHECK(simd::reduce_max(I7([](int i) { return i * 3 % 7; })) == 6);
  return 0;
}
