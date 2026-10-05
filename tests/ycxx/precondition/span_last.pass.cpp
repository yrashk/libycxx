// [span.sub]: last(count): "Hardened preconditions: count <= size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <span>
#include "violation.hpp"

int main() {
  int a[3] = {1, 2, 3};
  std::span<int> s(a);
  about_to_violate("span_last");
  keep(s.last(4));
  never_reached();
}
