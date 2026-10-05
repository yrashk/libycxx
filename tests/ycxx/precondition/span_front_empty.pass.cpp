// [span.elem]/6: front(): "Hardened preconditions: empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <span>
#include "violation.hpp"

int main() {
  std::span<int> s;
  about_to_violate("span_front_empty");
  keep(s.front());
  never_reached();
}
