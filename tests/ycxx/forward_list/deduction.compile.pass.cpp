// [forward.list.overview] deduction guides:
//   forward_list(InputIterator, InputIterator, Allocator = Allocator())
//     -> forward_list<iter-value-type<InputIterator>, Allocator>;
//   forward_list(from_range_t, R&&, Allocator = Allocator())
//     -> forward_list<ranges::range_value_t<R>, Allocator>;
// plus the implicit guides from the constructors (initializer_list, (n, value), copy).
#include <forward_list>
#include <memory>
#include <ranges>
#include <type_traits>
#include "test_allocators.hpp"
#include "test_iterators.hpp"

void f() {
  int arr[] = {1, 2, 3};
  std::forward_list a(arr, arr + 3);
  static_assert(std::is_same_v<decltype(a), std::forward_list<int>>);
  std::forward_list b(InputIter<int>(arr), InputIter<int>(arr + 3));
  static_assert(std::is_same_v<decltype(b), std::forward_list<int>>);
  std::forward_list c(arr, arr + 3, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(c), std::forward_list<int, MinimalAlloc<int>>>);
  std::forward_list d(std::from_range, arr);
  static_assert(std::is_same_v<decltype(d), std::forward_list<int>>);
  std::forward_list e(std::from_range, InputRange<int>{arr, arr + 3});
  static_assert(std::is_same_v<decltype(e), std::forward_list<int>>);
  std::forward_list g(std::from_range, arr, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(g), std::forward_list<int, MinimalAlloc<int>>>);
  std::forward_list h{1.5, 2.5};
  static_assert(std::is_same_v<decltype(h), std::forward_list<double>>);
  std::forward_list i(4u, 'x');
  static_assert(std::is_same_v<decltype(i), std::forward_list<char>>);
  std::forward_list j(a);
  static_assert(std::is_same_v<decltype(j), std::forward_list<int>>);
  std::forward_list k{a};  // copy deduction candidate is preferred
  static_assert(std::is_same_v<decltype(k), std::forward_list<int>>);
  std::forward_list l(c, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(l), std::forward_list<int, MinimalAlloc<int>>>);
  const long clongs[] = {1, 2};
  std::forward_list m(std::begin(clongs), std::end(clongs));
  static_assert(std::is_same_v<decltype(m), std::forward_list<long>>);
}

int main() { return 0; }
