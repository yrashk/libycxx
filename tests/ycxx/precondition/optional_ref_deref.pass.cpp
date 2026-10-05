// [optional.ref.observe]: optional<T&>::operator*(): "Hardened preconditions: has_value() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <optional>
#include "violation.hpp"

int main() {
  std::optional<int&> o;
  about_to_violate("optional_ref_deref");
  keep(*o);
  never_reached();
}
