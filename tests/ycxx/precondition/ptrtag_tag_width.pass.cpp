// [ptrtag.pair.cons]/3.2: "Preconditions: ... tag-bit-width(t) <= bits_requested is true."
// libycxx checks it with YCXX_HARDENED (DECISIONS §9). Death test (support/violation.hpp).
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <memory>
#include "violation.hpp"

int main() {
  alignas(16) static int x;
  std::pointer_tag_pair<int*, 2> ok(&x, 3u);
  keep(ok);
  about_to_violate("pointer_tag_pair tag wider than bits_requested");
  std::pointer_tag_pair<int*, 2> bad(&x, 4u);
  keep(bad);
  never_reached();
}
