// [sequence.reqmts]/122: a[n]: "Hardened preconditions: n < a.size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <deque>
#include "violation.hpp"

int main() {
  std::deque<int> d(2, 1);
  about_to_violate("deque_subscript");
  keep(d[2]);
  never_reached();
}
