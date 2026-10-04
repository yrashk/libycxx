// [alg.min.max]: min(initializer_list<T>) / ranges::min(r) "Returns: The smallest value in
// the input range. Returns a copy of the leftmost element when several elements are
// equivalent to the smallest."; max likewise: "Returns a copy of the leftmost element when
// several elements are equivalent to the largest." Return type T (std, ranges with an
// initializer_list) or range_value_t<R>; ranges::min/max accept input ranges. "Exactly
// ranges::distance(r) - 1 comparisons and twice as many applications of the projection".
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  static_assert(std::is_same_v<decltype(std::min({1, 2})), int>);
  static_assert(std::is_same_v<decltype(std::max({1, 2}, std::less<>{})), int>);
  static_assert(std::is_same_v<decltype(std::ranges::min({1, 2})), int>);
  int arr[] = {1, 2};
  static_assert(std::is_same_v<decltype(std::ranges::max(arr)), int>);
  if (std::min({4, 2, 8, 2}) != 2 || std::max({4, 2, 8, 8}) != 8) return false;
  if (std::min({5}) != 5 || std::max({5}) != 5) return false;
  if (std::min({4, 2, 8}, std::greater<>{}) != 8 || std::max({4, 2, 8}, std::greater<>{}) != 2) return false;
  // leftmost among equivalent elements
  if (std::min({KV{3, 0}, KV{1, 1}, KV{1, 2}, KV{2, 3}}).id != 1) return false;
  if (std::max({KV{3, 0}, KV{1, 1}, KV{3, 2}, KV{2, 3}}).id != 0) return false;
  if (std::ranges::min({KV{3, 0}, KV{1, 1}, KV{1, 2}}, {}, &KV::key).id != 1) return false;
  if (std::ranges::max({KV{3, 0}, KV{1, 1}, KV{3, 2}}, {}, &KV::key).id != 0) return false;
  // explicit template argument
  if (std::min<long>({3L, 1L}) != 1) return false;
  // ranges over arrays and input ranges, with projection
  KV ks[] = {{5, 0}, {2, 1}, {9, 2}, {2, 3}, {9, 4}};
  KV mn = std::ranges::min(ks, {}, &KV::key);
  KV mx = std::ranges::max(ks, {}, &KV::key);
  if (mn.id != 1 || mx.id != 2) return false;
  int vals[] = {7, 3, 9, 3};
  InputRange<int> ir{vals, vals + 4};
  if (std::ranges::min(ir) != 3) return false;
  InputRange<int> ir2{vals, vals + 4};
  if (std::ranges::max(ir2, std::ranges::greater{}) != 3) return false;
  // a range of prvalues (views::iota)
  if (std::ranges::max(std::views::iota(1, 6)) != 5 || std::ranges::min(std::views::iota(1, 6)) != 1) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  int comps = 0, projs = 0;
  (void)std::min({5, 3, 8, 1, 9, 2}, CountingLess{&comps});
  CHECK(comps == 5);
  comps = 0;
  (void)std::max({5, 3, 8, 1, 9, 2}, CountingLess{&comps});
  CHECK(comps == 5);
  comps = 0;
  int a[] = {5, 3, 8, 1, 9, 2, 7};
  CHECK(std::ranges::min(a, CountingLess{&comps}, CountingProj{&projs}) == 1);
  CHECK(comps == 6 && projs == 12);
  comps = projs = 0;
  CHECK(std::ranges::max(a, CountingLess{&comps}, CountingProj{&projs}) == 9);
  CHECK(comps == 6 && projs == 12);
  comps = projs = 0;
  CHECK(std::ranges::max({4}, CountingLess{&comps}, CountingProj{&projs}) == 4);
  CHECK(comps == 0 && projs == 0);
  return 0;
}
