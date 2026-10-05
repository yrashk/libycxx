// [span.cons]: span(It first, size_type count): "Hardened preconditions: If extent is not equal to
// dynamic_extent, then count is equal to extent."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <span>
#include "violation.hpp"

int main() {
  int a[3] = {1, 2, 3};
  about_to_violate("span_ctor_static_extent");
  std::span<int, 4> s(a + 0, 3);
  keep(s);
  never_reached();
}
