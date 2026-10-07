// [saferecl.rcu.domain.func]/4 with [saferecl.rcu.general]/5: rcu_barrier inside a region after
// the same thread retired an object in it would block for ever; libycxx checks it as a hardened
// precondition, as it does rcu_synchronize inside a region (DECISIONS §3).
// Death test (support/violation.hpp).
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
// FLAGS: -pthread
#include <rcu>
#include "violation.hpp"

static int a, b;
struct Del {
  void operator()(int*) const noexcept {}
};

int main() {
  std::rcu_retire(&a, Del{});
  std::rcu_default_domain().lock();
  std::rcu_barrier(); // a was retired before the region: fine
  std::rcu_retire(&b, Del{});
  about_to_violate("rcu_barrier inside a region after retiring in it");
  std::rcu_barrier();
  never_reached();
}
