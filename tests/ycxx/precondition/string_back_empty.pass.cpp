// [string.access]/9: back(): "Hardened preconditions: empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <string>
#include "violation.hpp"

int main() {
  const std::string s;
  about_to_violate("string_back_empty");
  keep(s.back());
  never_reached();
}
