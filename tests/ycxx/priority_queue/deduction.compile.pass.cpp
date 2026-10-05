// [priqueue.overview] deduction guides: (Compare, Container), iterator ranges with optional
// Compare and Container (vector of the value type by default), from_range with optional
// Compare, and the allocator forms (vector rebound to the allocator, or the given container).
// REQUIRES: exceptions
#include <queue>
#include <deque>
#include <functional>
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>
#include "test_allocators.hpp"

void f() {
  std::deque<long> d;
  std::priority_queue a(std::greater<long>(), d);
  static_assert(std::is_same_v<decltype(a), std::priority_queue<long, std::deque<long>, std::greater<long>>>);
  int arr[] = {1};
  std::priority_queue b(arr, arr + 1);
  static_assert(std::is_same_v<decltype(b), std::priority_queue<int>>);
  std::priority_queue c(arr, arr + 1, std::greater<int>());
  static_assert(std::is_same_v<decltype(c), std::priority_queue<int, std::vector<int>, std::greater<int>>>);
  std::priority_queue e(arr, arr + 1, std::greater<int>(), std::deque<int>());
  static_assert(std::is_same_v<decltype(e), std::priority_queue<int, std::deque<int>, std::greater<int>>>);
  std::priority_queue g(std::from_range, arr);
  static_assert(std::is_same_v<decltype(g), std::priority_queue<int>>);
  std::priority_queue h(std::from_range, arr, std::greater<int>(), MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(h), std::priority_queue<int, std::vector<int, MinimalAlloc<int>>, std::greater<int>>>);
  std::priority_queue i(arr, arr + 1, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(i), std::priority_queue<int, std::vector<int, MinimalAlloc<int>>>>);
  std::priority_queue j(std::less<long>(), d, std::allocator<long>());
  static_assert(std::is_same_v<decltype(j), std::priority_queue<long, std::deque<long>>>);
  std::priority_queue k(std::from_range, arr, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(k), std::priority_queue<int, std::vector<int, MinimalAlloc<int>>>>);
}

int main() { return 0; }
