// [span.elem]/1: operator[](idx) of a span with a static extent: "Hardened preconditions: idx <
// size() is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <span>
#include <cstddef>
#include "violation.hpp"

int main() {
  int a[3] = {1, 2, 3};
  std::span<int, 3> s(a);
  volatile std::size_t i = 3;
  about_to_violate("span_static_subscript");
  keep(s[i]);
  never_reached();
}
