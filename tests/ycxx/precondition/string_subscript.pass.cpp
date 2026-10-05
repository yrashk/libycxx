// [string.access]/1: operator[](pos): "Hardened preconditions: pos <= size() is true." (pos ==
// size() is allowed and reads the terminator; pos == size() + 1 is not).
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <string>
#include "violation.hpp"

int main() {
  std::string s = "abc";
  about_to_violate("string_subscript");
  keep(s[4]);
  never_reached();
}
