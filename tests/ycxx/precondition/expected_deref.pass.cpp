// [expected.object.obs]: operator*() &: "Hardened preconditions: has_value() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <expected>
#include "violation.hpp"

int main() {
  std::expected<int, int> e(std::unexpect, 1);
  about_to_violate("expected_deref");
  keep(*e);
  never_reached();
}
