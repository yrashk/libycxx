// Generic requirement checks extracted from tests/ycxx/containers/sequence_assign.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <initializer_list>
#include <ranges>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_iterators.hpp"

namespace reqs::sequence_assign {

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  using S = typename X::size_type;
  T arr[] = {val<T>(1), val<T>(2), val<T>(3), val<T>(4), val<T>(5)};
  static_assert(std::is_same_v<decltype(std::declval<X&>() = {val<T>(1)}), X&>);
  // basic_string's assign members return basic_string& instead ([string.assign]).
  using AR = std::conditional_t<requires { typename X::traits_type; }, X&, void>;
  static_assert(std::is_same_v<decltype(std::declval<X&>().assign(arr, arr)), AR>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().assign_range(arr)), AR>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().assign({val<T>(1)})), AR>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().assign(S(1), val<T>(1))), AR>);

  X a = make<X>({9, 9});
  X& r = (a = {val<T>(1), val<T>(2), val<T>(3)});
  if (&r != &a || !holds(a, {1, 2, 3})) return false;
  a = {val<T>(4)};
  if (!holds(a, {4})) return false;
  a = {};
  if (!a.empty()) return false;

  a.assign(arr, arr + 5);
  if (!holds(a, {1, 2, 3, 4, 5})) return false;
  a.assign(InputIter<T>(arr + 3), InputIter<T>(arr + 5));
  if (!holds(a, {4, 5})) return false;
  a.assign(ForwardIter<T>(arr), ForwardIter<T>(arr + 3));
  if (!holds(a, {1, 2, 3})) return false;
  a.assign(arr, arr);
  if (!a.empty()) return false;

  a.assign_range(InputRange<T>{arr, arr + 4});
  if (!holds(a, {1, 2, 3, 4})) return false;
  a.assign_range(ForwardRange<T>{arr + 4, arr + 5});
  if (!holds(a, {5})) return false;
  X src = make<X>({7, 6, 5, 4, 3, 2, 1, 0, 7, 6, 5, 4, 3, 2, 1, 0, 7, 6, 5, 4, 3, 2, 1, 0});
  a.assign_range(src);
  if (!(a == src)) return false;
  a.assign_range(std::ranges::subrange(arr, arr));
  if (!a.empty()) return false;

  a.assign({val<T>(2), val<T>(3)});
  if (!holds(a, {2, 3})) return false;
  a.assign(S(4), val<T>(8));
  if (!holds(a, {8, 8, 8, 8})) return false;
  a.assign(S(1), val<T>(6));
  if (!holds(a, {6})) return false;
  a.assign(S(0), val<T>(6));
  if (!a.empty()) return false;
  a.assign(S(30), val<T>(5));
  if (count_elems(a) != 30) return false;
  return true;
}

}  // namespace reqs::sequence_assign
