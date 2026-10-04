// [vector.overview] deduction guides:
//   vector(InputIterator, InputIterator, Allocator = Allocator())
//     -> vector<iter-value-type<InputIterator>, Allocator>;
//   vector(from_range_t, R&&, Allocator = Allocator()) -> vector<range_value_t<R>, Allocator>;
// [sequence.reqmts]/69.3: guides do not participate for non-iterators / non-allocators.
// The allocator-extended copy/move constructors take type_identity_t<Allocator>, so CTAD
// deduces from the vector argument.
#include <vector>
#include <list>
#include <ranges>
#include <type_traits>
#include "test_allocators.hpp"

void f() {
  int a[] = {1, 2};
  std::vector v1(a, a + 2);
  static_assert(std::is_same_v<decltype(v1), std::vector<int>>);
  std::list<double> l;
  std::vector v2(l.begin(), l.end(), MinimalAlloc<double>());
  static_assert(std::is_same_v<decltype(v2), std::vector<double, MinimalAlloc<double>>>);
  std::vector v3(std::from_range, std::views::iota(0L, 3L));
  static_assert(std::is_same_v<decltype(v3), std::vector<long>>);
  std::vector v4(std::from_range, a, std::allocator<int>());
  static_assert(std::is_same_v<decltype(v4), std::vector<int>>);
  std::vector v5{1, 2, 3};
  static_assert(std::is_same_v<decltype(v5), std::vector<int>>);
  std::vector v6(3, 'c');
  static_assert(std::is_same_v<decltype(v6), std::vector<char>>);
  std::vector v7(v5, std::allocator<int>());
  static_assert(std::is_same_v<decltype(v7), std::vector<int>>);
  std::vector<int, MinimalAlloc<int>> m;
  std::vector v8(m, MinimalAlloc<int>());
  static_assert(std::is_same_v<decltype(v8), std::vector<int, MinimalAlloc<int>>>);
  std::vector v9{v5};  // copy deduction candidate
  static_assert(std::is_same_v<decltype(v9), std::vector<int>>);
}

template <class... Args>
concept deducible = requires(Args... args) { std::vector(args...); };
static_assert(deducible<int*, int*>);
static_assert(!deducible<int*, int*, int>);  // int is not an allocator
