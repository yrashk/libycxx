// [view.interface.members]: back(): "Hardened preconditions: !empty() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <ranges>
#include <vector>
#include "violation.hpp"

int main() {
  std::vector<int> v;
  std::ranges::subrange r(v);
  about_to_violate("view_interface_back_empty");
  keep(r.back());
  never_reached();
}
