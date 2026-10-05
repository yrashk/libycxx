// [sequence.reqmts]/72: a.front() (forward_list provides it, [forward.list.overview]): "Hardened
// preconditions: a.empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <forward_list>
#include "violation.hpp"

int main() {
  std::forward_list<int> l;
  about_to_violate("forward_list_front_empty");
  keep(l.front());
  never_reached();
}
