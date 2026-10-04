// [simd.permute.dynamic], [simd.subscr]/4, [simd.mask.subscr]/4: permute(v, indices) and
// v[indices] give resize_t<I::size(), V> with elements v[indices[i]].
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
  // dynamic permute
  simd::vec<int, 3> idx([](int i) { return 7 - 3 * i; });  // 7 4 1
  auto dp = simd::permute(v, idx);
  static_assert(std::is_same_v<decltype(dp), simd::vec<int, 3>>);
  CHECK(eq(dp, {70, 40, 10}));
  CHECK(eq(v[idx], {70, 40, 10}));
  auto mp = mk[idx];
  static_assert(std::is_same_v<decltype(mp), simd::mask<int, 3>>);
  CHECK(!mp[0] && !mp[1] && mp[2]);

  return true;
}
static_assert(test());

int main() {
  test();
  return 0;
}
