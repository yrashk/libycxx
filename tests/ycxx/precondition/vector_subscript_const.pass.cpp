// [sequence.reqmts]/122: a[n] on a const vector: "Hardened preconditions: n < a.size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <vector>
#include "violation.hpp"

int main() {
  const std::vector<int> v(3, 7);
  about_to_violate("vector_subscript_const");
  keep(v[5]);
  never_reached();
}
