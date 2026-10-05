// [optional.observe]/1: operator->(): "Hardened preconditions: has_value() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <optional>
#include "violation.hpp"

int main() {
  struct S { int m; };
  std::optional<S> o;
  about_to_violate("optional_arrow");
  keep(o->m);
  never_reached();
}
