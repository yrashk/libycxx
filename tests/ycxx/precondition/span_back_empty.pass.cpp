// [span.elem]/9: back(): "Hardened preconditions: empty() is false."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <span>
#include "violation.hpp"

int main() {
  std::span<int> s;
  about_to_violate("span_back_empty");
  keep(s.back());
  never_reached();
}
