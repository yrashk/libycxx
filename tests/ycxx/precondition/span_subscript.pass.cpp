// [span.elem]/1: operator[](idx): "Hardened preconditions: idx < size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <span>
#include "violation.hpp"

int main() {
  int a[3] = {1, 2, 3};
  std::span<int> s(a);
  about_to_violate("span_subscript");
  keep(s[3]);
  never_reached();
}
