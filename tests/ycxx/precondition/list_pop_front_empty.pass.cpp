// [sequence.reqmts]/114: a.pop_front(): "Hardened preconditions: a.empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <list>
#include "violation.hpp"

int main() {
  std::list<int> l;
  about_to_violate("list_pop_front_empty");
  l.pop_front();
  never_reached();
}
