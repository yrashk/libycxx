// Generic requirement checks extracted from tests/ycxx/containers/sequence_insert.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_iterators.hpp"

namespace reqs::sequence_insert {

template <class X>
constexpr typename X::const_iterator at(const X& a, int k) {
  auto it = a.cbegin();
  std::advance(it, k);
  return it;
}

template <class X>
constexpr int pos(X& a, typename X::iterator it) {
  return static_cast<int>(std::distance(a.begin(), it));
}

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  using It = typename X::iterator;
  using S = typename X::size_type;
  T arr[] = {val<T>(20), val<T>(21), val<T>(22)};
  const T t = val<T>(9);
  static_assert(std::is_same_v<decltype(std::declval<X&>().insert(std::declval<X&>().cbegin(), t)), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().insert(std::declval<X&>().cbegin(), val<T>(1))), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().insert(std::declval<X&>().cbegin(), S(1), t)), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().insert(std::declval<X&>().cbegin(), arr, arr)), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().insert_range(std::declval<X&>().cbegin(), arr)), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().insert(std::declval<X&>().cbegin(), {t})), It>);

  X a = make<X>({1, 2, 3});
  It r = a.insert(at(a, 0), t);  // front, lvalue
  if (pos(a, r) != 0 || !(*r == t) || !holds(a, {9, 1, 2, 3})) return false;
  r = a.insert(at(a, 2), val<T>(8));  // middle, rvalue
  if (pos(a, r) != 2 || !holds(a, {9, 1, 8, 2, 3})) return false;
  r = a.insert(a.cend(), val<T>(7));  // end
  if (pos(a, r) != 5 || !holds(a, {9, 1, 8, 2, 3, 7})) return false;

  a = make<X>({1, 2});
  r = a.insert(at(a, 1), S(3), val<T>(5));
  if (pos(a, r) != 1 || !holds(a, {1, 5, 5, 5, 2})) return false;
  auto p = at(a, 2);
  r = a.insert(p, S(0), val<T>(5));
  if (pos(a, r) != 2 || !holds(a, {1, 5, 5, 5, 2})) return false;
  r = a.insert(a.cend(), S(40), val<T>(6));
  if (pos(a, r) != 5 || count_elems(a) != 45) return false;

  a = make<X>({1, 2});
  r = a.insert(at(a, 1), arr, arr + 3);
  if (pos(a, r) != 1 || !holds(a, {1, 20, 21, 22, 2})) return false;
  r = a.insert(at(a, 0), InputIter<T>(arr), InputIter<T>(arr + 2));
  if (pos(a, r) != 0 || !holds(a, {20, 21, 1, 20, 21, 22, 2})) return false;
  r = a.insert(a.cend(), ForwardIter<T>(arr + 2), ForwardIter<T>(arr + 3));
  if (pos(a, r) != 7 || !holds(a, {20, 21, 1, 20, 21, 22, 2, 22})) return false;
  r = a.insert(at(a, 3), arr, arr);
  if (pos(a, r) != 3 || count_elems(a) != 8) return false;

  a = make<X>({1, 2});
  r = a.insert_range(at(a, 1), InputRange<T>{arr, arr + 3});
  if (pos(a, r) != 1 || !holds(a, {1, 20, 21, 22, 2})) return false;
  r = a.insert_range(at(a, 0), ForwardRange<T>{arr + 1, arr + 2});
  if (pos(a, r) != 0 || !holds(a, {21, 1, 20, 21, 22, 2})) return false;
  X src = make<X>({30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48});
  r = a.insert_range(a.cend(), src);
  if (pos(a, r) != 6 || count_elems(a) != 25) return false;
  r = a.insert_range(at(a, 4), std::ranges::subrange(arr, arr));
  if (pos(a, r) != 4 || count_elems(a) != 25) return false;

  a = make<X>({1, 2});
  r = a.insert(at(a, 1), {val<T>(3), val<T>(4)});
  if (pos(a, r) != 1 || !holds(a, {1, 3, 4, 2})) return false;
  r = a.insert(at(a, 2), std::initializer_list<T>{});
  if (pos(a, r) != 2 || !holds(a, {1, 3, 4, 2})) return false;

  X e;
  r = e.insert(e.cend(), t);
  if (r != e.begin() || !holds(e, {9})) return false;
  return true;
}

}  // namespace reqs::sequence_insert
