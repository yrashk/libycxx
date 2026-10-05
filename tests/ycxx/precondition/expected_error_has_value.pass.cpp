// [expected.object.obs]: error() &: "Hardened preconditions: has_value() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <expected>
#include "violation.hpp"

int main() {
  std::expected<int, int> e(5);
  about_to_violate("expected_error_has_value");
  keep(e.error());
  never_reached();
}
