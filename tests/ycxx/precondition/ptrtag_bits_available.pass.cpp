// [ptrtag.bits]/4: pointer_bits_available: "Preconditions: alignment is a power of two."
// libycxx checks it with YCXX_HARDENED (DECISIONS §9). Death test (support/violation.hpp).
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <cstddef>
#include <memory>
#include "violation.hpp"

int main() {
  volatile std::size_t a = 16;
  keep(std::pointer_bits_available(a));
  a = 12;
  about_to_violate("pointer_bits_available of a non-power of two");
  keep(std::pointer_bits_available(a));
  never_reached();
}
