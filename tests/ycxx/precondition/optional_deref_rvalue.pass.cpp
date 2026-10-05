// [optional.observe]/7: operator*() &&: "Hardened preconditions: has_value() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <optional>
#include <utility>
#include "violation.hpp"

int main() {
  std::optional<int> o;
  about_to_violate("optional_deref_rvalue");
  int x = *std::move(o);
  keep(x);
  never_reached();
}
