// [flat.set.defn], [flat.multiset.defn] deduction guides: from a container (with optional
// Compare / Allocator, with and without sorted_unique / sorted_equivalent), from iterator
// ranges, from_range (vector rebound to the allocator) and initializer_list.
#include <flat_set>
#include <deque>
#include <functional>
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>

void f() {
  std::deque<int> d{1};
  std::flat_set a(d);
  static_assert(std::is_same_v<decltype(a), std::flat_set<int, std::less<int>, std::deque<int>>>);
  std::flat_set b(std::sorted_unique, d, std::greater<int>());
  static_assert(std::is_same_v<decltype(b), std::flat_set<int, std::greater<int>, std::deque<int>>>);
  std::flat_set c(d, std::allocator<int>());
  static_assert(std::is_same_v<decltype(c), std::flat_set<int, std::less<int>, std::deque<int>>>);
  int arr[] = {1, 2};
  std::flat_set e(arr, arr + 2);
  static_assert(std::is_same_v<decltype(e), std::flat_set<int>>);
  std::flat_set g(std::from_range, arr);
  static_assert(std::is_same_v<decltype(g), std::flat_set<int>>);
  std::flat_set h{1L, 2L};
  static_assert(std::is_same_v<decltype(h), std::flat_set<long>>);
  std::flat_multiset i(std::sorted_equivalent, arr, arr + 2);
  static_assert(std::is_same_v<decltype(i), std::flat_multiset<int>>);
  std::flat_multiset j(d);
  static_assert(std::is_same_v<decltype(j), std::flat_multiset<int, std::less<int>, std::deque<int>>>);
}

int main() { return 0; }
