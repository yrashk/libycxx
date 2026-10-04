// Generic requirement checks extracted from tests/ycxx/containers/sequence_emplace.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"

namespace reqs::sequence_emplace {

template <class X>
constexpr bool generic() {
  using T = typename X::value_type;
  using It = typename X::iterator;
  static_assert(std::is_same_v<decltype(std::declval<X&>().emplace(std::declval<X&>().cbegin(), val<T>(1))), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().emplace_back(val<T>(1))), typename X::reference>);
  X a = make<X>({1, 2, 3});
  It r = a.emplace(cnth(a, 1), val<T>(5));
  if (r != nth(a, 1) || !holds(a, {1, 5, 2, 3})) return false;
  r = a.emplace(a.cend(), val<T>(6));
  if (r != std::prev(a.end()) || !holds(a, {1, 5, 2, 3, 6})) return false;
  r = a.emplace(a.cbegin(), val<T>(7));
  if (r != a.begin() || !holds(a, {7, 1, 5, 2, 3, 6})) return false;
  // argument referring to an element of a, with and without reallocation
  for (int k = 0; k < 40; ++k) a.emplace(cnth(a, 1), a.back());
  if (count_elems(a) != 46 || !(*nth(a, 1) == val<T>(6)) || !(*nth(a, 40) == val<T>(6)) || !(*nth(a, 41) == val<T>(1))) return false;
  a.emplace(a.cbegin(), *nth(a, 2));
  if (!(*a.begin() == val<T>(6))) return false;

  X b;
  auto&& ref = b.emplace_back(val<T>(4));
  if (!(ref == val<T>(4))) return false;
  for (int k = 0; k < 40; ++k) {
    auto&& r2 = b.emplace_back(b.front());  // refers into b, reallocation along the way
    if (!(r2 == val<T>(4))) return false;
  }
  if (count_elems(b) != 41) return false;
  return true;
}

}  // namespace reqs::sequence_emplace
