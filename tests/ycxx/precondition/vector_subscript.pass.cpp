// [sequence.reqmts]/122: a[n]: "Hardened preconditions: n < a.size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <vector>
#include "violation.hpp"

int main() {
  std::vector<int> v(3, 7);
  about_to_violate("vector_subscript");
  keep(v[3]);
  never_reached();
}
