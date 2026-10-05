// [string.view.modifiers]/1: remove_prefix(n): "Hardened preconditions: n <= size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <string_view>
#include "violation.hpp"

int main() {
  std::string_view sv = "abc";
  about_to_violate("string_view_remove_prefix");
  sv.remove_prefix(4);
  keep(sv);
  never_reached();
}
