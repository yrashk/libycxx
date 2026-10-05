// [sequence.reqmts]/118: a.pop_back(): "Hardened preconditions: a.empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <vector>
#include "violation.hpp"

int main() {
  std::vector<int> v;
  about_to_violate("vector_pop_back_empty");
  v.pop_back();
  never_reached();
}
