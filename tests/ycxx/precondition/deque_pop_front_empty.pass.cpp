// [sequence.reqmts]/114: a.pop_front(): "Hardened preconditions: a.empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <deque>
#include "violation.hpp"

int main() {
  std::deque<int> d;
  about_to_violate("deque_pop_front_empty");
  d.pop_front();
  never_reached();
}
