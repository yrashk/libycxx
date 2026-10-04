// [list.overview] deduction guides:
//   list(InputIterator, InputIterator, Allocator = Allocator())
//     -> list<iter-value-type<InputIterator>, Allocator>;
//   list(from_range_t, R&&, Allocator = Allocator())
//     -> list<ranges::range_value_t<R>, Allocator>;
// plus the implicit guides from the constructors (initializer_list, (n, value), copy).
#include <list>
#include <memory>
#include <ranges>
#include <type_traits>
#include "test_allocators.hpp"
#include "test_iterators.hpp"

void f() {
  int arr[] = {1, 2, 3};
  std::list a(arr, arr + 3);
  static_assert(std::is_same_v<decltype(a), std::list<int>>);
  std::list b(InputIter<int>(arr), InputIter<int>(arr + 3));
  static_assert(std::is_same_v<decltype(b), std::list<int>>);
  std::list c(arr, arr + 3, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(c), std::list<int, MinimalAlloc<int>>>);
  std::list d(std::from_range, arr);
  static_assert(std::is_same_v<decltype(d), std::list<int>>);
  std::list e(std::from_range, InputRange<int>{arr, arr + 3});
  static_assert(std::is_same_v<decltype(e), std::list<int>>);
  std::list g(std::from_range, arr, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(g), std::list<int, MinimalAlloc<int>>>);
  std::list h{1.5, 2.5};
  static_assert(std::is_same_v<decltype(h), std::list<double>>);
  std::list i(4u, 'x');
  static_assert(std::is_same_v<decltype(i), std::list<char>>);
  std::list j(a);
  static_assert(std::is_same_v<decltype(j), std::list<int>>);
  std::list k{a};  // copy deduction candidate is preferred
  static_assert(std::is_same_v<decltype(k), std::list<int>>);
  std::list l(c, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(l), std::list<int, MinimalAlloc<int>>>);
  const long clongs[] = {1, 2};
  std::list m(std::begin(clongs), std::end(clongs));
  static_assert(std::is_same_v<decltype(m), std::list<long>>);
}

int main() { return 0; }
