// [queue.defn] deduction guides: queue(Container), queue(InputIterator, InputIterator)
// (deque of the value type), queue(from_range, R), queue(Container, Allocator),
// queue(InputIterator, InputIterator, Allocator) and queue(from_range, R, Allocator) (deque
// with that allocator). [container.adaptors.general]/6: guides do not participate when an
// allocator-like type is deduced for Container or a non-allocator for Allocator.
#include <queue>
#include <deque>
#include <list>
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>
#include "test_allocators.hpp"

void f() {
  std::vector<long> v;
  std::queue a(v);
  static_assert(std::is_same_v<decltype(a), std::queue<long, std::vector<long>>>);
  std::queue b(v, std::allocator<long>());
  static_assert(std::is_same_v<decltype(b), std::queue<long, std::vector<long>>>);
  int arr[] = {1};
  std::queue c(arr, arr + 1);
  static_assert(std::is_same_v<decltype(c), std::queue<int>>);
  std::queue d(arr, arr + 1, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(d), std::queue<int, std::deque<int, MinimalAlloc<int>>>>);
  std::queue e(std::from_range, arr);
  static_assert(std::is_same_v<decltype(e), std::queue<int>>);
  std::queue g(std::from_range, arr, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(g), std::queue<int, std::deque<int, MinimalAlloc<int>>>>);
  std::queue h(std::list<char>{});
  static_assert(std::is_same_v<decltype(h), std::queue<char, std::list<char>>>);
  std::queue i(a);
  static_assert(std::is_same_v<decltype(i), decltype(a)>);
}

int main() { return 0; }
