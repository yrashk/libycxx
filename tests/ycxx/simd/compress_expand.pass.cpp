// [simd.permute.mask]: compress(v, selector[, fill_value]) packs the selected elements to the
// front in order, the remaining elements being fill_value (or unspecified without it);
// expand(v, selector, original = {}) puts v[0], v[1], ... at the selected positions in ascending
// order and original[i] elsewhere. Both exist for basic_mask too.
#include <simd>
#include <array>
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include "check.hpp"

namespace simd = std::simd;
using V = simd::vec<int, 8>;
using M = V::mask_type;

template <class W>
constexpr bool eq(const W& w, std::initializer_list<typename W::value_type> l) {
  if (static_cast<std::size_t>(W::size()) != l.size()) return false;
  int i = 0;
  for (auto x : l)
    if (w[i++] != x) return false;
  return true;
}

constexpr bool test() {
  V v = V([](int i) { return 10 * i; });  // 0 10 ... 70
  M mk([](int i) { return i < 3; });
  // compress / expand
  M sel([](int i) { return i % 3 == 0; });  // 0 3 6
  auto c = simd::compress(v, sel, -1);
  CHECK(eq(c, {0, 30, 60, -1, -1, -1, -1, -1}));
  auto c2 = simd::compress(v, sel);
  CHECK(c2[0] == 0 && c2[1] == 30 && c2[2] == 60);
  auto e = simd::expand(V([](int i) { return i + 1; }), sel, V(-5));
  CHECK(eq(e, {1, -5, -5, 2, -5, -5, 3, -5}));
  auto e0 = simd::expand(V(9), sel);
  CHECK(eq(e0, {9, 0, 0, 9, 0, 0, 9, 0}));
  // [simd.mask.ctor]/8: basic_mask(unsigned) sets element i from bit i, so 0b10101010 selects
  // indices 1, 3, 5, 7; mk is true at 0-2, so the result is mk[1], mk[3], mk[5], mk[7] = true,
  // false, false, false, then the fill value.
  auto mc = simd::compress(mk, M(0b10101010u), false);
  CHECK(mc[0] && !mc[1] && !mc[2] && !mc[3] && !mc[4] && !mc[7]);

  return true;
}
static_assert(test());

int main() {
  test();
  return 0;
}
