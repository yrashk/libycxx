// [sequence.reqmts]/76: a.back(): "Hardened preconditions: a.empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <inplace_vector>
#include "violation.hpp"

int main() {
  std::inplace_vector<int, 8> v;
  about_to_violate("inplace_vector_back_empty");
  keep(v.back());
  never_reached();
}
