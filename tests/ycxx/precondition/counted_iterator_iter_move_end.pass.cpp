// [counted.iter.cust]/1: iter_move(i): "Hardened preconditions: i.length > 0 is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <iterator>
#include "violation.hpp"

int main() {
  int a[2] = {1, 2};
  std::counted_iterator<int*> it(a, 0);
  about_to_violate("counted_iterator_iter_move_end");
  keep(std::ranges::iter_move(it));
  never_reached();
}
