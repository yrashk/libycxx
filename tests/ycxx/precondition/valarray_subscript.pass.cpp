// [valarray.access]: operator[](n): "Hardened preconditions: n < size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <valarray>
#include "violation.hpp"

int main() {
  std::valarray<int> v(3);
  about_to_violate("valarray_subscript");
  keep(v[3]);
  never_reached();
}
