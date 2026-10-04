// [stack.defn] deduction guides: stack(Container), stack(InputIterator, InputIterator)
// (deque of the value type), stack(from_range, R), stack(Container, Allocator),
// stack(InputIterator, InputIterator, Allocator) and stack(from_range, R, Allocator) (deque
// with that allocator). [container.adaptors.general]/6: guides do not participate when an
// allocator-like type is deduced for Container or a non-allocator for Allocator.
#include <stack>
#include <deque>
#include <list>
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>
#include "test_allocators.hpp"

void f() {
  std::vector<long> v;
  std::stack a(v);
  static_assert(std::is_same_v<decltype(a), std::stack<long, std::vector<long>>>);
  std::stack b(v, std::allocator<long>());
  static_assert(std::is_same_v<decltype(b), std::stack<long, std::vector<long>>>);
  int arr[] = {1};
  std::stack c(arr, arr + 1);
  static_assert(std::is_same_v<decltype(c), std::stack<int>>);
  std::stack d(arr, arr + 1, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(d), std::stack<int, std::deque<int, MinimalAlloc<int>>>>);
  std::stack e(std::from_range, arr);
  static_assert(std::is_same_v<decltype(e), std::stack<int>>);
  std::stack g(std::from_range, arr, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(g), std::stack<int, std::deque<int, MinimalAlloc<int>>>>);
  std::stack h(std::list<char>{});
  static_assert(std::is_same_v<decltype(h), std::stack<char, std::list<char>>>);
  std::stack i(a);
  static_assert(std::is_same_v<decltype(i), decltype(a)>);
}

int main() { return 0; }
