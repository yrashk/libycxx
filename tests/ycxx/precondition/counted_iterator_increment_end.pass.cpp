// [counted.iter.nav]/1: operator++(): "Hardened preconditions: length > 0 is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <iterator>
#include "violation.hpp"

int main() {
  int a[2] = {1, 2};
  std::counted_iterator<int*> it(a, 0);
  about_to_violate("counted_iterator_increment_end");
  ++it;
  keep(it);
  never_reached();
}
