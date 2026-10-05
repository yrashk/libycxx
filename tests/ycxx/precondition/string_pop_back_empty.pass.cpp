// [string.modifiers]: pop_back(): "Hardened preconditions: empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <string>
#include "violation.hpp"

int main() {
  std::string s;
  about_to_violate("string_pop_back_empty");
  s.pop_back();
  never_reached();
}
