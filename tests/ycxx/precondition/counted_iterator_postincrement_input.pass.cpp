// [counted.iter.nav]/3: operator++(int) of an input iterator: "Hardened preconditions: length > 0 is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <iterator>
#include <sstream>
#include "violation.hpp"

int main() {
  std::istringstream in("1 2");
  std::counted_iterator<std::istream_iterator<int>> it(std::istream_iterator<int>(in), 0);
  about_to_violate("counted_iterator_postincrement_input");
  it++;
  never_reached();
}
