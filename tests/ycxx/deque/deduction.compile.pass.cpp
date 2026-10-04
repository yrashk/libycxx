// [deque.overview] deduction guides:
//   deque(InputIterator, InputIterator, Allocator = Allocator())
//     -> deque<iter-value-type<InputIterator>, Allocator>;
//   deque(from_range_t, R&&, Allocator = Allocator())
//     -> deque<ranges::range_value_t<R>, Allocator>;
// plus the implicit guides from the constructors (initializer_list, (n, value), copy).
#include <deque>
#include <memory>
#include <ranges>
#include <type_traits>
#include "test_allocators.hpp"
#include "test_iterators.hpp"

void f() {
  int arr[] = {1, 2, 3};
  std::deque a(arr, arr + 3);
  static_assert(std::is_same_v<decltype(a), std::deque<int>>);
  std::deque b(InputIter<int>(arr), InputIter<int>(arr + 3));
  static_assert(std::is_same_v<decltype(b), std::deque<int>>);
  std::deque c(arr, arr + 3, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(c), std::deque<int, MinimalAlloc<int>>>);
  std::deque d(std::from_range, arr);
  static_assert(std::is_same_v<decltype(d), std::deque<int>>);
  std::deque e(std::from_range, InputRange<int>{arr, arr + 3});
  static_assert(std::is_same_v<decltype(e), std::deque<int>>);
  std::deque g(std::from_range, arr, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(g), std::deque<int, MinimalAlloc<int>>>);
  std::deque h{1.5, 2.5};
  static_assert(std::is_same_v<decltype(h), std::deque<double>>);
  std::deque i(4u, 'x');
  static_assert(std::is_same_v<decltype(i), std::deque<char>>);
  std::deque j(a);
  static_assert(std::is_same_v<decltype(j), std::deque<int>>);
  std::deque k{a};  // copy deduction candidate is preferred
  static_assert(std::is_same_v<decltype(k), std::deque<int>>);
  std::deque l(c, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(l), std::deque<int, MinimalAlloc<int>>>);
  const long clongs[] = {1, 2};
  std::deque m(std::begin(clongs), std::end(clongs));
  static_assert(std::is_same_v<decltype(m), std::deque<long>>);
}

int main() { return 0; }
