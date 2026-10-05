// [sequence.reqmts]/72: a.front(): "Hardened preconditions: a.empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <vector>
#include "violation.hpp"

int main() {
  std::vector<int> v;
  about_to_violate("vector_front_empty");
  keep(v.front());
  never_reached();
}
