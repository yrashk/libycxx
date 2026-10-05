// [span.sub]: first<Count>(): "Mandates: Count <= Extent is true." (dynamic extent: satisfied)
// "Hardened preconditions: Count <= size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <span>
#include "violation.hpp"

int main() {
  int a[3] = {1, 2, 3};
  std::span<int> s(a);
  about_to_violate("span_first_static");
  keep(s.first<4>());
  never_reached();
}
