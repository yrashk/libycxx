// Generic requirement checks extracted from tests/ycxx/containers/container_three_way.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <compare>
#include <type_traits>
#include "container_values.hpp"

namespace reqs::container_three_way {

template <class X, class Cat>
constexpr bool test() {
  static_assert(std::is_same_v<decltype(std::declval<const X&>() <=> std::declval<const X&>()), Cat>);
  X a = make<X>({1, 2, 3});
  X same = make<X>({1, 2, 3});
  X prefix = make<X>({1, 2});
  X bigger_first = make<X>({2});
  X smaller_last = make<X>({1, 2, 0});
  X e;
  if ((a <=> same) != 0 || !(a == same)) return false;
  if ((prefix <=> a) >= 0 || (a <=> prefix) <= 0) return false;
  if ((a <=> bigger_first) >= 0 || (bigger_first <=> a) <= 0) return false;  // element beats length
  if ((smaller_last <=> a) >= 0) return false;
  if ((e <=> a) >= 0 || (e <=> X()) != 0) return false;
  if (!(prefix < a) || !(a > prefix) || !(a <= same) || !(a >= same) || a < same) return false;
  return true;
}

}  // namespace reqs::container_three_way
