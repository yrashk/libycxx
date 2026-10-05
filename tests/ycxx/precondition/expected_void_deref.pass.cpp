// [expected.void.obs]: operator*(): "Hardened preconditions: has_value() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <expected>
#include "violation.hpp"

int main() {
  std::expected<void, int> e(std::unexpect, 1);
  about_to_violate("expected_void_deref");
  *e;
  never_reached();
}
