// [bitset.members]: operator[](pos): "Hardened preconditions: pos < size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <bitset>
#include "violation.hpp"

int main() {
  std::bitset<8> b;
  about_to_violate("bitset_subscript");
  auto r = b[8];
  keep(r);
  never_reached();
}
