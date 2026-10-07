// [counted.iter.nav]/8: operator+=(n): "Hardened preconditions: n <= length is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <iterator>
#include "violation.hpp"

int main() {
  int a[4] = {1, 2, 3, 4};
  std::counted_iterator<int*> it(a, 2);
  about_to_violate("counted_iterator_advance");
  it += 3;
  keep(it);
  never_reached();
}
