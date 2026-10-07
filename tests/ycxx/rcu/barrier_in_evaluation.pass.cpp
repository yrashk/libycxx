// rcu_barrier() called from inside a scheduled evaluation (a deleter).
//   [saferecl.rcu.domain.func]/4: rcu_barrier "May evaluate any scheduled evaluations in dom.
//     For any evaluation that happens before the call to rcu_barrier and that schedules an
//     evaluation E in dom, blocks until E has been evaluated." /5: "The evaluation of any such E
//     strongly happens before the return from rcu_barrier."
//   [saferecl.rcu.general]/6: "Each scheduled evaluation is evaluated at most once."
// Every evaluation scheduled before the nested call (the rest of the batch the deleter belongs
// to, what is still queued, what the deleter itself retired) is evaluated when it returns. The
// deleter's own evaluation, in progress, cannot be (STATUS "Draft issues noticed").
// Each scenario runs in a child process with an alarm: a barrier that never returns ends it.
// FLAGS: -pthread
#include <atomic>
#include <rcu>
#include <thread>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

constexpr int N = 8;
static int deleted[N];   // how many times object i was deleted
static bool seen_by_nested[N]; // what the nested barrier left evaluated
static int nested_calls = 0;

struct Del {
  void operator()(int* p) const noexcept;
};
static int objs[N];
static int extra;          // retired by the deleter of objs[0] before its barrier
static int extra_deleted;

void Del::operator()(int* p) const noexcept {
  const long i = p - objs;
  ++deleted[i];
  if (i == 0) {
    struct ExtraDel {
      void operator()(int*) const noexcept { ++extra_deleted; }
    };
    std::rcu_retire(&extra, ExtraDel{});
    ++nested_calls;
    std::rcu_barrier();
    for (int k = 0; k < N; ++k)
      seen_by_nested[k] = deleted[k] == 1;
    CHECK(extra_deleted == 1);
  }
}

static void barrier_in_deleter() {
  for (int i = 0; i < N; ++i)
    std::rcu_retire(&objs[i], Del{});
  std::rcu_barrier(); // evaluates objs[0], whose deleter calls rcu_barrier
  CHECK(nested_calls == 1);
  for (int i = 1; i < N; ++i)
    CHECK(seen_by_nested[i]); // evaluated by the nested barrier
  for (int i = 0; i < N; ++i)
    CHECK(deleted[i] == 1);    // at most once
  CHECK(extra_deleted == 1);
  std::rcu_barrier();
  for (int i = 0; i < N; ++i)
    CHECK(deleted[i] == 1);
}

// The same with another thread's region: the nested barrier waits for the readers that hold
// back what it must evaluate, while the evaluation runs.
static std::atomic<int> phase{0};
static int late;
static int late_deleted;
struct Waiter {
  void operator()(int*) const noexcept {
    struct LateDel {
      void operator()(int*) const noexcept { ++late_deleted; }
    };
    // Retired while the reader's region (begun earlier) is open: the nested barrier must wait
    // for the region's end.
    std::rcu_retire(&late, LateDel{});
    phase.store(2);
    std::rcu_barrier();
    CHECK(late_deleted == 1);
    CHECK(phase.load() == 3); // the reader's region has ended
  }
};
static int waited;

static void barrier_in_deleter_waits_for_readers() {
  // Retired before the reader's region begins: the region does not hold it back.
  std::rcu_retire(&waited, Waiter{});
  std::thread reader([] {
    std::rcu_default_domain().lock();
    phase.store(1);
    while (phase.load() != 2)
      std::this_thread::yield();
    usleep(20000);
    phase.store(3);
    std::rcu_default_domain().unlock();
  });
  while (phase.load() != 1)
    std::this_thread::yield();
  // The evaluation of Waiter runs during the region and retires `late`, which the region holds
  // back: the barrier inside the evaluation waits for the region's end.
  std::thread evaluator([] { std::rcu_barrier(); });
  evaluator.join();
  reader.join();
  CHECK(late_deleted == 1);
}

static void in_child(void (*f)(), const char* name) {
  const pid_t pid = fork();
  CHECK(pid >= 0);
  if (pid == 0) {
    alarm(30);
    f();
    _exit(0);
  }
  int status = 0;
  CHECK(waitpid(pid, &status, 0) == pid);
  if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
    dprintf(2, "%s: rcu_barrier did not return\n", name);
  else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    dprintf(2, "%s failed (status %#x)\n", name, status);
  CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

int main() {
  in_child(barrier_in_deleter, "rcu_barrier inside a deleter");
  in_child(barrier_in_deleter_waits_for_readers, "rcu_barrier inside a deleter, with a reader");
  return 0;
}
