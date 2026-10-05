// [span.sub]: subspan(offset): "Hardened preconditions: offset <= size() && ..." with offset >
// size().
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <span>
#include "violation.hpp"

int main() {
  int a[3] = {1, 2, 3};
  std::span<int> s(a);
  about_to_violate("span_subspan_offset");
  keep(s.subspan(4));
  never_reached();
}
