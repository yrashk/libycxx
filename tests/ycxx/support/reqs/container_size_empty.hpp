// Generic requirement checks extracted from tests/ycxx/containers/container_size_empty.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <iterator>
#include <limits>
#include <type_traits>
#include "container_values.hpp"

namespace reqs::container_size_empty {

template <class X>
constexpr bool test() {
  using S = typename X::size_type;
  using D = typename X::difference_type;
  static_assert(std::is_same_v<decltype(std::declval<const X&>().size()), S>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().max_size()), S>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().empty()), bool>);
  X c;
  for (int n = 0; n < 40; ++n) {
    const X& cc = c;
    if (cc.size() != static_cast<S>(std::distance(cc.begin(), cc.end()))) return false;
    if (cc.size() != static_cast<S>(n)) return false;
    if (cc.empty() != (cc.begin() == cc.end())) return false;
    if (cc.empty() != (n == 0)) return false;
    if (cc.max_size() < cc.size()) return false;
    if (cc.max_size() > static_cast<S>(std::numeric_limits<D>::max())) return false;
    c.insert(c.end(), val<typename X::value_type>(n));
  }
  while (!c.empty()) {
    c.erase(c.begin());
    if (c.size() != static_cast<S>(std::distance(c.begin(), c.end()))) return false;
  }
  return c.size() == 0;
}

}  // namespace reqs::container_size_empty
