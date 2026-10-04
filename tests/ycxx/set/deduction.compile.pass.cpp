// [set.overview], [multiset.overview] deduction guides: iterator range (iter-value-type),
// from_range (range_value_t), initializer_list<Key>, each with optional Compare and
// Allocator, and the allocator-only forms using less<Key>.
#include <set>
#include <functional>
#include <ranges>
#include <type_traits>
#include "test_allocators.hpp"
#include "test_iterators.hpp"

using A = MinimalAlloc<int>;

void f() {
  int arr[] = {3, 1, 2};
  std::set a(arr, arr + 3);
  static_assert(std::is_same_v<decltype(a), std::set<int>>);
  std::set b(InputIter<int>(arr), InputIter<int>(arr + 3), std::greater<int>());
  static_assert(std::is_same_v<decltype(b), std::set<int, std::greater<int>>>);
  std::set c(arr, arr + 3, A());
  static_assert(std::is_same_v<decltype(c), std::set<int, std::less<int>, A>>);
  std::set d(std::from_range, arr);
  static_assert(std::is_same_v<decltype(d), std::set<int>>);
  std::set e(std::from_range, arr, std::greater<>(), A());
  static_assert(std::is_same_v<decltype(e), std::set<int, std::greater<>, A>>);
  std::set g(std::from_range, arr, A());
  static_assert(std::is_same_v<decltype(g), std::set<int, std::less<int>, A>>);
  std::set h{1L, 2L};
  static_assert(std::is_same_v<decltype(h), std::set<long>>);
  std::set i({1, 2}, std::greater<int>());
  static_assert(std::is_same_v<decltype(i), std::set<int, std::greater<int>>>);
  std::set j({1, 2}, A());
  static_assert(std::is_same_v<decltype(j), std::set<int, std::less<int>, A>>);
  std::multiset k(arr, arr + 3);
  static_assert(std::is_same_v<decltype(k), std::multiset<int>>);
  std::multiset l(std::from_range, arr, A());
  static_assert(std::is_same_v<decltype(l), std::multiset<int, std::less<int>, A>>);
  std::multiset m{'a', 'b'};
  static_assert(std::is_same_v<decltype(m), std::multiset<char>>);
  std::set n{a};  // copy deduction
  static_assert(std::is_same_v<decltype(n), std::set<int>>);
}

int main() { return 0; }
