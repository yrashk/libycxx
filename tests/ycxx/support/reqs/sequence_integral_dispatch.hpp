// Generic requirement checks extracted from tests/ycxx/containers/sequence_integral_dispatch.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <type_traits>
#include "container_values.hpp"

namespace reqs::sequence_integral_dispatch {

template <class X, class I>
constexpr bool test() {
  using T = typename X::value_type;
  // Variables, not literals: an integer literal 0 would also be a null pointer constant.
  I n0 = 0, n1 = 1, n2 = 2, n3 = 3, n4 = 4, c65 = 65, c66 = 66, c67 = 67;
  X a(n3, c65);
  if (a.size() != 3 || *nth(a, 0) != T(65) || *nth(a, 2) != T(65)) return false;
  a.assign(n2, c66);
  if (a.size() != 2 || *nth(a, 1) != T(66)) return false;
  auto it = a.insert(cnth(a, 1), n4, c67);
  if (a.size() != 6 || it != nth(a, 1) || *nth(a, 1) != T(67) || *nth(a, 4) != T(67) || *nth(a, 5) != T(66)) return false;
  X b(n0, n1);
  if (!b.empty()) return false;
  return true;
}

}  // namespace reqs::sequence_integral_dispatch
