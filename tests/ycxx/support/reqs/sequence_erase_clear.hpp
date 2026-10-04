// Generic requirement checks extracted from tests/ycxx/containers/sequence_erase_clear.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <type_traits>
#include <utility>
#include "container_values.hpp"

namespace reqs::sequence_erase_clear {

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  using It = typename X::iterator;
  static_assert(std::is_same_v<decltype(std::declval<X&>().erase(std::declval<X&>().cbegin())), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().erase(std::declval<X&>().cbegin(), std::declval<X&>().cend())), It>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().clear()), void>);

  X a = make<X>({1, 2, 3, 4, 5, 6});
  It r = a.erase(cnth(a, 1));
  if (r != nth(a, 1) || !(*r == val<T>(3)) || !holds(a, {1, 3, 4, 5, 6})) return false;
  r = a.erase(a.cbegin());
  if (r != a.begin() || !holds(a, {3, 4, 5, 6})) return false;
  r = a.erase(std::prev(a.cend()));
  if (r != a.end() || !holds(a, {3, 4, 5})) return false;

  a = make<X>({1, 2, 3, 4, 5, 6});
  r = a.erase(cnth(a, 1), cnth(a, 3));
  if (r != nth(a, 1) || !(*r == val<T>(4)) || !holds(a, {1, 4, 5, 6})) return false;
  r = a.erase(cnth(a, 2), cnth(a, 2));
  if (r != nth(a, 2) || !holds(a, {1, 4, 5, 6})) return false;
  r = a.erase(cnth(a, 2), a.cend());
  if (r != a.end() || !holds(a, {1, 4})) return false;
  r = a.erase(a.cbegin(), a.cend());
  if (r != a.end() || r != a.begin() || !a.empty()) return false;

  X e;
  r = e.erase(e.cbegin(), e.cend());
  if (r != e.end()) return false;

  X c = make<X>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23});
  c.clear();
  if (!c.empty() || c.size() != 0 || c.begin() != c.end()) return false;
  c.clear();
  if (!c.empty()) return false;
  c.insert(c.end(), val<T>(1));  // still usable
  return holds(c, {1});
}

}  // namespace reqs::sequence_erase_clear
