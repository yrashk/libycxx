// [saferecl.rcu.domain.func]/2: rcu_synchronize "If the call to rcu_synchronize does not strongly
// happen before the lock opening an RCU protection region R on dom, blocks until the unlock
// closing R happens." Inside R that is for ever; libycxx checks it as a hardened precondition
// (DECISIONS §3). Death test (support/violation.hpp).
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
// FLAGS: -pthread
#include <rcu>
#include "violation.hpp"

int main() {
  std::rcu_synchronize(); // outside any region: returns
  std::rcu_default_domain().lock();
  about_to_violate("rcu_synchronize inside a region");
  std::rcu_synchronize();
  never_reached();
}
