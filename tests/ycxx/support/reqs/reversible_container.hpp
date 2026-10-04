// Generic requirement checks extracted from tests/ycxx/containers/reversible_container.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <iterator>
#include <type_traits>
#include "container_values.hpp"

namespace reqs::reversible_container {

template <class X>
constexpr bool test() {
  using RI = typename X::reverse_iterator;
  using CRI = typename X::const_reverse_iterator;
  X a = make<X>({1, 2, 3, 4});
  const X& ca = a;
  static_assert(std::is_same_v<decltype(a.rbegin()), RI> && std::is_same_v<decltype(a.rend()), RI>);
  static_assert(std::is_same_v<decltype(ca.rbegin()), CRI> && std::is_same_v<decltype(ca.rend()), CRI>);
  static_assert(std::is_same_v<decltype(a.crbegin()), CRI> && std::is_same_v<decltype(a.crend()), CRI>);
  if (a.rbegin() != RI(a.end()) || a.rend() != RI(a.begin())) return false;
  if (ca.rbegin() != CRI(ca.end()) || ca.rend() != CRI(ca.begin())) return false;
  if (a.crbegin() != ca.rbegin() || a.crend() != ca.rend()) return false;
  if (a.rbegin().base() != a.end() || a.rend().base() != a.begin()) return false;
  int expect = 4;
  for (auto it = a.rbegin(); it != a.rend(); ++it, --expect)
    if (!(*it == val<typename X::value_type>(expect))) return false;
  if (expect != 0) return false;
  expect = 4;
  for (auto it = a.crbegin(); it != a.crend(); ++it, --expect)
    if (!(*it == val<typename X::value_type>(expect))) return false;
  if (std::distance(a.rbegin(), a.rend()) != 4) return false;
  X e;
  if (e.rbegin() != e.rend() || e.crbegin() != e.crend()) return false;
  return true;
}

}  // namespace reqs::reversible_container
