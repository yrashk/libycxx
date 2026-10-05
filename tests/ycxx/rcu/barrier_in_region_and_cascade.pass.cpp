// rcu_barrier() called inside a region of RCU protection that began after the objects were
// retired, and deleters that retire further objects (cascading reclamation).
//   [saferecl.rcu.domain.func]/4: rcu_barrier "May evaluate any scheduled evaluations in dom.
//     For any evaluation that happens before the call to rcu_barrier and that schedules an
//     evaluation E in dom, blocks until E has been evaluated." It has no precondition.
//   [saferecl.rcu.general]/5: "Given a region of RCU protection R on a domain dom and given an
//     evaluation E that scheduled another evaluation F in dom, if E does not strongly happen
//     before the start of R, the end of R strongly happens before evaluating F." Here every
//     retire strongly happens before the start of the region (same thread, sequenced before),
//     so nothing requires the region to end first: the barrier inside it evaluates the
//     deleters and returns (single thread: no other evaluation is scheduled).
//   [saferecl.rcu.base]/9 retire and /6-8 rcu_retire may be called from a scheduled
//     evaluation (a deleter); such a retire happens before a later rcu_barrier call, which
//     then evaluates it. [saferecl.rcu.general]/6: "Each scheduled evaluation is evaluated at
//     most once."
// Each scenario runs in a child process with an alarm: a barrier that never returns ends it.
// FLAGS: -pthread
#include <rcu>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

constexpr int N = 300;

struct Node;
struct Cascade {
  void operator()(Node* p) const noexcept;
};
struct Node : std::rcu_obj_base<Node, Cascade> {
  int deletions = 0;
  Node* next = nullptr;  // retired by this node's deleter
};
static Node nodes[N];
void Cascade::operator()(Node* p) const noexcept {
  ++p->deletions;
  if (p->next) p->next->retire();
}

static int plain[N];
static int plain_deletions[N];
struct PlainDeleter {
  void operator()(int* p) const noexcept {
    const long i = p - plain;
    ++plain_deletions[i];
    if (i + 1 < N) std::rcu_retire(&plain[i + 1], PlainDeleter{});
  }
};

struct Count {
  void operator()(int* p) const noexcept { ++*p; }
};

static void barrier_in_region() {
  int counted = 0;
  nodes[0].retire();  // (next is null: no cascade)
  std::rcu_retire(&counted, Count{});
  std::rcu_default_domain().lock();
  std::rcu_barrier();
  const bool done = nodes[0].deletions == 1 && counted == 1;
  std::rcu_default_domain().unlock();
  CHECK(done);
}

static void cascade() {
  for (int i = 0; i + 1 < N; ++i) nodes[i].next = &nodes[i + 1];
  nodes[0].retire();
  std::rcu_retire(&plain[0], PlainDeleter{});
  // Each barrier evaluates at least what was scheduled before it: after N barriers every node
  // of both chains has been deleted.
  for (int round = 0; round < N; ++round) std::rcu_barrier();
  for (int i = 0; i < N; ++i) {
    CHECK(nodes[i].deletions == 1);
    CHECK(plain_deletions[i] == 1);
  }
  std::rcu_barrier();
  for (int i = 0; i < N; ++i) CHECK(nodes[i].deletions == 1 && plain_deletions[i] == 1);
}

static void in_child(void (*f)(), const char* name) {
  const pid_t pid = fork();
  CHECK(pid >= 0);
  if (pid == 0) {
    alarm(20);
    f();
    _exit(0);
  }
  int status = 0;
  CHECK(waitpid(pid, &status, 0) == pid);
  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) dprintf(2, "%s: rcu_barrier did not return\n", name);
  else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) dprintf(2, "%s failed (status %#x)\n", name, status);
  CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

int main() {
  in_child(cascade, "cascading retire from deleters");
  in_child(barrier_in_region, "rcu_barrier inside a region begun after the retires");
  return 0;
}
