// [sequence.reqmts]/122 (array is a sequence container, [array.overview]): a[n]: "Hardened
// preconditions: n < a.size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
// COUNTERPART: libcxx:containers/sequences/array/assert.indexing.pass.cpp
#include <array>
#include <cstddef>
#include "violation.hpp"

int main() {
  std::array<int, 3> a{1, 2, 3};
  volatile std::size_t n = 3;
  about_to_violate("array_subscript");
  keep(a[n]);
  never_reached();
}
