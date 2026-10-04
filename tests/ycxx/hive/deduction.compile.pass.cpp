// [hive.overview] deduction guides: from iterator ranges and from_range, each with an
// optional hive_limits and Allocator.
#include <hive>
#include <memory>
#include <ranges>
#include <type_traits>
#include "test_allocators.hpp"

void f() {
  int arr[] = {1, 2};
  std::hive a(arr, arr + 2);
  static_assert(std::is_same_v<decltype(a), std::hive<int>>);
  std::hive b(arr, arr + 2, std::hive_limits(8, 16));
  static_assert(std::is_same_v<decltype(b), std::hive<int>>);
  std::hive c(arr, arr + 2, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(c), std::hive<int, MinimalAlloc<int>>>);
  std::hive d(std::from_range, arr);
  static_assert(std::is_same_v<decltype(d), std::hive<int>>);
  std::hive e(std::from_range, arr, std::hive_limits(8, 16), MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(e), std::hive<int, MinimalAlloc<int>>>);
  std::hive g{1L};
  static_assert(std::is_same_v<decltype(g), std::hive<long>>);
}

int main() { return 0; }
