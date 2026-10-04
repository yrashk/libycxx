// Generic requirement checks extracted from tests/ycxx/containers/container_equality.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <type_traits>
#include "container_values.hpp"

namespace reqs::container_equality {

template <class X>
constexpr bool generic() {
  static_assert(std::is_same_v<decltype(std::declval<const X&>() == std::declval<const X&>()), bool>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>() != std::declval<const X&>()), bool>);
  X a = make<X>({1, 2, 3});
  X b = make<X>({1, 2, 3});
  X c = make<X>({1, 2, 3});
  X shorter = make<X>({1, 2});
  X longer = make<X>({1, 2, 3, 4});
  X diff = make<X>({1, 2, 4});
  X e1, e2;
  // reflexive, symmetric, transitive
  if (!(a == a) || !(a == b) || !(b == a) || !(b == c) || !(a == c)) return false;
  if (a == shorter || shorter == a || a == longer || longer == a || e1 == a || a == e1) return false;
  if (!(e1 == e2)) return false;
  if (!(a != shorter) || !(a != longer) || a != b || e1 != e2) return false;
  if constexpr (!std::is_same_v<typename X::value_type, bool>) {
    if (a == diff || !(a != diff)) return false;
  }
  return true;
}

}  // namespace reqs::container_equality
