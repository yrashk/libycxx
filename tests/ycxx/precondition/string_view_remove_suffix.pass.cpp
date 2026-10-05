// [string.view.modifiers]/3: remove_suffix(n): "Hardened preconditions: n <= size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <string_view>
#include "violation.hpp"

int main() {
  std::string_view sv = "abc";
  about_to_violate("string_view_remove_suffix");
  sv.remove_suffix(4);
  keep(sv);
  never_reached();
}
