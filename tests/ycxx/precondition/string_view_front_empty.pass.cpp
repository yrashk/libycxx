// [string.view.access]: front(): "Hardened preconditions: empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <string_view>
#include "violation.hpp"

int main() {
  std::string_view sv;
  about_to_violate("string_view_front_empty");
  keep(sv.front());
  never_reached();
}
