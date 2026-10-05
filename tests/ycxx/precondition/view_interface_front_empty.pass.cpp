// [view.interface.members]: front(): "Hardened preconditions: !empty() is true." (subrange derives
// from view_interface)
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <ranges>
#include <vector>
#include "violation.hpp"

int main() {
  std::vector<int> v;
  std::ranges::subrange r(v);
  about_to_violate("view_interface_front_empty");
  keep(r.front());
  never_reached();
}
