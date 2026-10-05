// [sequence.reqmts]/76: a.back(): "Hardened preconditions: a.empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <list>
#include "violation.hpp"

int main() {
  std::list<int> l;
  about_to_violate("list_back_empty");
  keep(l.back());
  never_reached();
}
