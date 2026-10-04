// [map.overview] and [multimap.overview] deduction guides: from an iterator range of pairs
// (iter-key-type removes const from the first type, iter-mapped-type is the second type),
// from from_range with range-key-type / range-mapped-type, from initializer_list<pair<Key,
// T>>, each with optional Compare and Allocator, and the allocator-only forms that use
// less<Key>. [associative.reqmts.general]/181: a guide does not participate when an
// allocator type is deduced for Compare, or a non-allocator for Allocator, or a non-iterator
// for InputIterator.
#include <map>
#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"

using P = std::pair<const int, long>;
using A = MinimalAlloc<P>;

void f() {
  std::pair<int, long> arr[] = {{1, 2}, {3, 4}};
  P carr[] = {{1, 2}};
  std::map a(arr, arr + 2);
  static_assert(std::is_same_v<decltype(a), std::map<int, long>>);
  std::map b(carr, carr + 1);
  static_assert(std::is_same_v<decltype(b), std::map<int, long>>);
  std::map c(arr, arr + 2, std::greater<int>());
  static_assert(std::is_same_v<decltype(c), std::map<int, long, std::greater<int>>>);
  std::map d(arr, arr + 2, std::greater<int>(), A());
  static_assert(std::is_same_v<decltype(d), std::map<int, long, std::greater<int>, A>>);
  std::map e(arr, arr + 2, A());  // allocator-only guide
  static_assert(std::is_same_v<decltype(e), std::map<int, long, std::less<int>, A>>);
  std::map g(std::from_range, arr);
  static_assert(std::is_same_v<decltype(g), std::map<int, long>>);
  std::map h(std::from_range, carr, std::greater<>());
  static_assert(std::is_same_v<decltype(h), std::map<int, long, std::greater<>>>);
  std::map i(std::from_range, arr, A());
  static_assert(std::is_same_v<decltype(i), std::map<int, long, std::less<int>, A>>);
  std::map j{std::pair{1, 2L}, std::pair{3, 4L}};
  static_assert(std::is_same_v<decltype(j), std::map<int, long>>);
  std::map k({std::pair{1, 2L}}, std::greater<int>());
  static_assert(std::is_same_v<decltype(k), std::map<int, long, std::greater<int>>>);
  std::map l({std::pair{1, 2L}}, A());
  static_assert(std::is_same_v<decltype(l), std::map<int, long, std::less<int>, A>>);
  std::multimap m(arr, arr + 2);
  static_assert(std::is_same_v<decltype(m), std::multimap<int, long>>);
  std::multimap n(std::from_range, carr, A());
  static_assert(std::is_same_v<decltype(n), std::multimap<int, long, std::less<int>, A>>);
  std::multimap o{std::pair{1, 2L}, std::pair{1, 3L}};
  static_assert(std::is_same_v<decltype(o), std::multimap<int, long>>);
  std::multimap q(arr, arr + 2, std::greater<int>(), A());
  static_assert(std::is_same_v<decltype(q), std::multimap<int, long, std::greater<int>, A>>);
  std::map r(a);
  static_assert(std::is_same_v<decltype(r), std::map<int, long>>);
}

int main() { return 0; }
