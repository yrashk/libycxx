// [mdspan.mdspan.members]: operator[](indices) of a rank-1 mdspan: "Hardened preconditions: I is a
// multidimensional index in extents()."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <mdspan>
#include "violation.hpp"

int main() {
  int a[4] = {};
  std::mdspan m(a, 4);
  about_to_violate("mdspan_subscript_rank1");
  keep(m[4]);
  never_reached();
}
