// Generic requirement checks extracted from tests/ycxx/containers/container_swap.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"

namespace reqs::container_swap {

template <class X>
constexpr bool contents() {
  static_assert(std::is_same_v<decltype(std::declval<X&>().swap(std::declval<X&>())), void>);
  X a = make<X>({1, 2, 3});
  X b = make<X>({4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24});
  a.swap(b);
  if (!holds(b, {1, 2, 3}) || !holds(a, {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24}))
    return false;
  using std::swap;
  swap(a, b);
  if (!holds(a, {1, 2, 3}) || count_elems(b) != 21) return false;
  X e;
  a.swap(e);
  if (!a.empty() || !holds(e, {1, 2, 3})) return false;
  swap(e, a);
  if (!e.empty() || !holds(a, {1, 2, 3})) return false;
  a.swap(a);
  if (!holds(a, {1, 2, 3})) return false;
  return true;
}

template <class X>
constexpr bool iterators_follow() {
  X a = make<X>({1, 2, 3});
  X b = make<X>({4, 5});
  auto ia = nth(a, 1);
  auto pa = std::addressof(*ia);
  auto ib = b.begin();
  auto pb = std::addressof(*ib);
  a.swap(b);
  // ia now refers to an element of b, ib to an element of a
  if (std::addressof(*ia) != pa || std::addressof(*ib) != pb) return false;
  if (!(*ia == val<typename X::value_type>(2)) || !(*ib == val<typename X::value_type>(4))) return false;
  if (std::addressof(*nth(b, 1)) != pa || std::addressof(*a.begin()) != pb) return false;
  ++ia;
  ++ia;
  if (ia != b.end()) return false;
  return true;
}

}  // namespace reqs::container_swap
