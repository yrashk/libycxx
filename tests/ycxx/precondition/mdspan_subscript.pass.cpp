// [mdspan.mdspan.members]: operator[](indices): "Hardened preconditions: I is a multidimensional
// index in extents()."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <mdspan>
#include "violation.hpp"

int main() {
  int a[6] = {};
  std::mdspan m(a, 2, 3);
  about_to_violate("mdspan_subscript");
  keep(m[1, 3]);
  never_reached();
}
