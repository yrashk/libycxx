// [container.reqmts]/42-47 and [container.opt.reqmts]: map == map compares sizes and then
// the elements (pairs: key and mapped value) in iteration order; <=> is
// lexicographical_compare_three_way with synth-three-way over pair<const Key, T>, so its
// result type is the common comparison category of Key and T. Same for multimap, where
// equivalent elements are compared in their stored order.
#include <map>
#include <compare>
#include <type_traits>
#include "check.hpp"

constexpr bool test() {
  using M = std::map<int, double>;
  static_assert(std::is_same_v<decltype(M() <=> M()), std::partial_ordering>);
  static_assert(std::is_same_v<decltype(std::map<int, int>() <=> std::map<int, int>()), std::strong_ordering>);
  M a{{1, 1.0}, {2, 2.0}};
  M b{{2, 2.0}, {1, 1.0}};
  M c{{1, 1.0}, {2, 2.5}};
  M d{{1, 1.0}};
  M e{{0, 9.0}, {5, 0.0}};
  if (!(a == b) || a != b || a == c || a == d) return false;
  if (!(a < c) || !(d < a) || !(e < a) || (a <=> b) != 0) return false;
  std::multimap<int, int> m1{{1, 1}, {1, 2}}, m2{{1, 2}, {1, 1}}, m3{{1, 1}, {1, 2}};
  if (m1 == m2 || !(m1 == m3) || !(m1 < m2)) return false;
  static_assert(std::is_same_v<decltype(m1 <=> m2), std::strong_ordering>);
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
