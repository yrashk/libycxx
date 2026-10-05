// [sequence.reqmts]/122 (vector<bool> is a sequence container, [vector.bool.pspc]): a[n]: "Hardened
// preconditions: n < a.size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <vector>
#include "violation.hpp"

int main() {
  std::vector<bool> v(4, true);
  about_to_violate("vector_bool_subscript");
  bool b = v[4];
  keep(b);
  never_reached();
}
