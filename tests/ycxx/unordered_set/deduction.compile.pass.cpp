// [unord.set.overview], [unord.multiset.overview] deduction guides: iterator range,
// from_range and initializer_list, with optional bucket count, Hash, Pred and Allocator,
// and the (n, Allocator) / (n, Hash, Allocator) forms.
#include <unordered_set>
#include <functional>
#include <ranges>
#include <type_traits>
#include "test_allocators.hpp"

struct H {
  std::size_t operator()(int i) const { return static_cast<std::size_t>(i); }
};
using A = MinimalAlloc<int>;

void f() {
  int arr[] = {1, 2, 3};
  std::unordered_set a(arr, arr + 3);
  static_assert(std::is_same_v<decltype(a), std::unordered_set<int>>);
  std::unordered_set b(arr, arr + 3, 8, H());
  static_assert(std::is_same_v<decltype(b), std::unordered_set<int, H>>);
  std::unordered_set c(arr, arr + 3, 8, A());
  static_assert(std::is_same_v<decltype(c), std::unordered_set<int, std::hash<int>, std::equal_to<int>, A>>);
  std::unordered_set d(std::from_range, arr, 8, H(), std::equal_to<>(), A());
  static_assert(std::is_same_v<decltype(d), std::unordered_set<int, H, std::equal_to<>, A>>);
  std::unordered_set e{1L, 2L};
  static_assert(std::is_same_v<decltype(e), std::unordered_set<long>>);
  std::unordered_set g({1, 2}, 8, H(), A());
  static_assert(std::is_same_v<decltype(g), std::unordered_set<int, H, std::equal_to<int>, A>>);
  std::unordered_multiset h(std::from_range, arr);
  static_assert(std::is_same_v<decltype(h), std::unordered_multiset<int>>);
  std::unordered_multiset i(arr, arr + 3, 8, H(), A());
  static_assert(std::is_same_v<decltype(i), std::unordered_multiset<int, H, std::equal_to<int>, A>>);
}

int main() { return 0; }
