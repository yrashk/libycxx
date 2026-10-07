// [ptrtag.pair.overalign]/2.2: "Preconditions: ... p == nullptr ||
// is_sufficiently_aligned<PromisedAlignment>(p) is true."
// libycxx checks it with YCXX_HARDENED (DECISIONS §9). Death test (support/violation.hpp).
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <memory>
#include "violation.hpp"

int main() {
  alignas(64) static char buf[128];
  using P = std::pointer_tag_pair<char*, 3>;
  keep(P::from_overaligned<64>(buf + 64, 7u));
  keep(P::from_overaligned<64>(static_cast<char*>(nullptr), 7u));
  about_to_violate("from_overaligned with a pointer not aligned to PromisedAlignment");
  keep(P::from_overaligned<64>(buf + 32, 0u)); // 32-aligned only: the low bits are free, the promise is not kept
  never_reached();
}
