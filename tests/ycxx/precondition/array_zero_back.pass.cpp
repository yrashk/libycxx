// [sequence.reqmts]/76 with [array.zero]: back() of a zero-sized array: "Hardened preconditions:
// a.empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
// COUNTERPART: libcxx:containers/sequences/array/assert.back.pass.cpp
#include <array>
#include "violation.hpp"

int main() {
  std::array<int, 0> a{};
  about_to_violate("array_zero_back");
  keep(a.back());
  never_reached();
}
