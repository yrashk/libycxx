// [simd.permute.static]: permute<N>(v, idxmap) returns resize_t<N, V> whose ith element is
// v[idxmap(i)] or v[idxmap(i, V::size())] (/1.1: the two-argument form when well-formed);
// zero_element gives T(); N defaults to V::size() (/5); masks permute the same way.
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
  // static permute
  auto rev = simd::permute(v, [](int i, int n) { return n - 1 - i; });
  static_assert(std::is_same_v<decltype(rev), V>);
  CHECK(eq(rev, {70, 60, 50, 40, 30, 20, 10, 0}));
  auto evens = simd::permute<4>(v, [](int i) { return 2 * i; });
  static_assert(std::is_same_v<decltype(evens), simd::vec<int, 4>>);
  CHECK(eq(evens, {0, 20, 40, 60}));
  auto dup = simd::permute<10>(v, [](int i) { return i / 2; });
  CHECK(eq(dup, {0, 0, 10, 10, 20, 20, 30, 30, 40, 40}));
  auto z = simd::permute(v, [](int i) { return i % 2 ? simd::zero_element : i; });
  CHECK(eq(z, {0, 0, 20, 0, 40, 0, 60, 0}));
  auto mrev = simd::permute(mk, [](int i, int n) { return n - 1 - i; });
  static_assert(std::is_same_v<decltype(mrev), M>);
  CHECK(mrev[7] && mrev[5] && !mrev[4] && !mrev[0]);

  return true;
}

static_assert(test());

int main() {
  test();
  return 0;
}
