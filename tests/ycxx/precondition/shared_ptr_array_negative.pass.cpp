// [util.smartptr.shared.obs]: operator[](i): "Hardened preconditions: i >= 0. If T is U[N], i < N."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <memory>
#include "violation.hpp"

int main() {
  std::shared_ptr<int[]> p(new int[3]{});
  about_to_violate("shared_ptr_array_negative");
  keep(p[-1]);
  never_reached();
}
