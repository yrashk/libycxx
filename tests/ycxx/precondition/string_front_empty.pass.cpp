// [string.access]/7: front(): "Hardened preconditions: empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <string>
#include "violation.hpp"

int main() {
  std::string s;
  about_to_violate("string_front_empty");
  keep(s.front());
  never_reached();
}
