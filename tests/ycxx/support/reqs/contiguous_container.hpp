// Generic requirement checks extracted from tests/ycxx/containers/contiguous_container.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <iterator>
#include <memory>
#include <type_traits>
#include "container_values.hpp"

namespace reqs::contiguous_container {

template <class X>
constexpr bool test() {
  static_assert(std::contiguous_iterator<typename X::iterator>);
  static_assert(std::contiguous_iterator<typename X::const_iterator>);
  static_assert(std::random_access_iterator<typename X::iterator>);
  static_assert(std::derived_from<typename std::iterator_traits<typename X::iterator>::iterator_category,
                                  std::random_access_iterator_tag>);
  static_assert(std::derived_from<typename std::iterator_traits<typename X::const_iterator>::iterator_category,
                                  std::random_access_iterator_tag>);
  X a = make<X>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25});
  const X& ca = a;
  auto b = a.begin();
  auto cb = ca.begin();
  for (int k = 0; k < 25; ++k) {
    auto it = b + k;
    if (std::to_address(it) != std::to_address(b) + k) return false;
    if (std::to_address(it) != std::addressof(*it)) return false;
    if (std::to_address(cb + k) != std::to_address(it)) return false;
    if (a.data() + k != std::addressof(a[static_cast<typename X::size_type>(k)])) return false;
  }
  if (a.data() != std::to_address(a.begin()) || ca.data() != std::to_address(ca.cbegin())) return false;
  if (std::to_address(a.end()) != a.data() + 25) return false;  // past-the-end
  return true;
}

}  // namespace reqs::contiguous_container
