// [sequence.reqmts]/122 (inplace_vector is a sequence container, [inplace.vector.overview]): a[n]:
// "Hardened preconditions: n < a.size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <inplace_vector>
#include "violation.hpp"

int main() {
  std::inplace_vector<int, 8> v(2, 5);
  about_to_violate("inplace_vector_subscript");
  keep(v[2]);
  never_reached();
}
