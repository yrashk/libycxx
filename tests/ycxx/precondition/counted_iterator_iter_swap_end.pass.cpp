// [counted.iter.cust]/3: iter_swap(x, y): "Hardened preconditions: Both x.length > 0 and y.length > 0 are true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <iterator>
#include "violation.hpp"

int main() {
  int a[2] = {1, 2};
  std::counted_iterator<int*> x(a, 1), y(a + 1, 0);
  about_to_violate("counted_iterator_iter_swap_end");
  std::ranges::iter_swap(x, y);
  never_reached();
}
