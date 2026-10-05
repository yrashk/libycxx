// [util.smartptr.shared.obs]: operator[](i): "Hardened preconditions: ... If T is U[N], i < N."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <memory>
#include "violation.hpp"

int main() {
  std::shared_ptr<int[3]> p(new int[3]{});
  about_to_violate("shared_ptr_bounded_array");
  keep(p[3]);
  never_reached();
}
