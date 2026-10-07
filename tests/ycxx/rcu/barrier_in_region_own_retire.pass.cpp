// rcu_barrier() inside a region of RCU protection after the same thread retired an object in it.
//   [saferecl.rcu.domain.func]/4: "For any evaluation that happens before the call to
//     rcu_barrier and that schedules an evaluation E in dom, blocks until E has been evaluated."
//   [saferecl.rcu.general]/5: "if E does not strongly happen before the start of R, the end of
//     R strongly happens before evaluating F."
// The retire is sequenced before the call and after the start of the region, which only ends
// after the call returns: the barrier blocks for ever (and the deleter never runs). There is no
// precondition and no exception; libycxx's hardened mode reports it instead
// (precondition/rcu_barrier_in_region_own_retire.pass.cpp, DECISIONS §3).
// REQUIRES: !hardened
// FLAGS: -pthread
#include <rcu>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

static int obj;
static volatile int deleted;
struct Del {
  void operator()(int*) const noexcept { deleted = 1; }
};

int main() {
  const pid_t pid = fork();
  CHECK(pid >= 0);
  if (pid == 0) {
    alarm(2);
    std::rcu_default_domain().lock();
    std::rcu_retire(&obj, Del{});
    std::rcu_barrier();
    _exit(deleted ? 4 : 5); // returned: wrong either way
  }
  int status = 0;
  CHECK(waitpid(pid, &status, 0) == pid);
  if (!WIFSIGNALED(status))
    dprintf(2, "rcu_barrier returned (status %#x)\n", status);
  CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM);
  return 0;
}
