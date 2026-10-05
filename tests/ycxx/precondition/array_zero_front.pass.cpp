// [sequence.reqmts]/72 with [array.zero]: front() of a zero-sized array: "Hardened preconditions:
// a.empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
// COUNTERPART: libcxx:containers/sequences/array/assert.front.pass.cpp
#include <array>
#include "violation.hpp"

int main() {
  std::array<int, 0> a{};
  about_to_violate("array_zero_front");
  keep(a.front());
  never_reached();
}
