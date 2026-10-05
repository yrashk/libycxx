// [bitset.members]: operator[](pos) const: "Hardened preconditions: pos < size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <bitset>
#include "violation.hpp"

int main() {
  const std::bitset<8> b;
  about_to_violate("bitset_subscript_const");
  bool v = b[9];
  keep(v);
  never_reached();
}
