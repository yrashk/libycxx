// [expected.void.obs]: error() &: "Hardened preconditions: has_value() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <expected>
#include "violation.hpp"

int main() {
  std::expected<void, int> e;
  about_to_violate("expected_void_error");
  keep(e.error());
  never_reached();
}
