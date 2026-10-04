// [flat.map.defn], [flat.multimap.defn] deduction guides: from (KeyContainer,
// MappedContainer[, Compare][, Allocator]) with and without sorted_unique /
// sorted_equivalent; from iterator ranges (iter-key-type / iter-mapped-type); from_range
// (vector containers rebound to the allocator, default allocator<byte>); initializer_list.
#include <flat_map>
#include <deque>
#include <functional>
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>

void f() {
  std::vector<int> k{1};
  std::deque<long> v{2};
  std::flat_map a(k, v);
  static_assert(std::is_same_v<decltype(a), std::flat_map<int, long, std::less<int>, std::vector<int>, std::deque<long>>>);
  std::flat_map b(k, v, std::greater<int>());
  static_assert(std::is_same_v<decltype(b), std::flat_map<int, long, std::greater<int>, std::vector<int>, std::deque<long>>>);
  std::flat_map c(std::sorted_unique, k, v, std::allocator<int>());
  static_assert(std::is_same_v<decltype(c), std::flat_map<int, long, std::less<int>, std::vector<int>, std::deque<long>>>);
  std::pair<int, long> arr[] = {{1, 2}};
  std::flat_map d(arr, arr + 1);
  static_assert(std::is_same_v<decltype(d), std::flat_map<int, long>>);
  std::flat_map e(std::from_range, arr);
  static_assert(std::is_same_v<decltype(e), std::flat_map<int, long>>);
  std::flat_map g{std::pair{1, 2L}};
  static_assert(std::is_same_v<decltype(g), std::flat_map<int, long>>);
  std::flat_map h(std::sorted_unique, {std::pair{1, 2L}}, std::greater<int>());
  static_assert(std::is_same_v<decltype(h), std::flat_map<int, long, std::greater<int>>>);
  std::flat_multimap i(k, v);
  static_assert(std::is_same_v<decltype(i), std::flat_multimap<int, long, std::less<int>, std::vector<int>, std::deque<long>>>);
  std::flat_multimap j(std::sorted_equivalent, arr, arr + 1);
  static_assert(std::is_same_v<decltype(j), std::flat_multimap<int, long>>);
}

int main() { return 0; }
