// The noexcept operations of RCU and hazard pointers while operator new fails: every call from
// the k-th on fails (memory is exhausted), for k = 1, 2, ... until the scenario makes no
// allocation that fails. Each run is a fresh child process (fork), so the default RCU domain
// and the hazard pointer machinery are first used during the failures. An allocation failure
// escaping a noexcept function calls std::terminate ([except.spec]/5), which kills the child.
//   [saferecl.rcu.domain.func]/1 rcu_default_domain() noexcept: "A reference to the same
//     object is returned every time"; [saferecl.rcu.domain.general] lock(), try_lock() (true,
//     [saferecl.rcu.domain.locks]), unlock() noexcept;
//   [saferecl.rcu.base]/9 rcu_obj_base<T, D>::retire(d, dom) noexcept: "Evaluates deleter =
//     std::move(d) and schedules the evaluation of the expression deleter(addressof(x)) in the
//     domain dom" (no allocation failure is permitted to it, unlike rcu_retire);
//   [saferecl.rcu.domain.func]/4-5 rcu_barrier() noexcept: "For any evaluation that happens
//     before the call to rcu_barrier and that schedules an evaluation E in dom, blocks until E
//     has been evaluated": after it, every object retired so far has been deleted, exactly once
//     ([saferecl.rcu.general]/6: "Each scheduled evaluation is evaluated at most once");
//     /2 rcu_synchronize() noexcept;
//   /6-9 rcu_retire(p, d, dom): "May allocate memory", "Throws: bad_alloc ..."; [Note 2: "If
//     rcu_retire exits via an exception, no evaluation is scheduled"]: the deleter of p runs
//     after rcu_barrier exactly when rcu_retire returned normally;
//   [saferecl.hp.base]/7 hazard_pointer_obj_base<T, D>::retire(d) noexcept;
//     [saferecl.hp.holder.mem] protect, try_protect, reset_protection noexcept;
//     [saferecl.hp.general]/5-6: a retired object is reclaimed at most once, and not while a
//     hazard pointer protects it since before it was retired.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <atomic>
#include <cstdlib>
#include <hazard_pointer>
#include <mutex>
#include <new>
#include <rcu>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

static long fail_from = 0, calls = 0;
static bool failed = false;
static void* allocate(std::size_t n, std::size_t align, bool nothrow) {
  if (fail_from && __atomic_add_fetch(&calls, 1, __ATOMIC_RELAXED) >= fail_from) {
    __atomic_store_n(&failed, true, __ATOMIC_RELAXED);
    if (nothrow) return nullptr;
    throw std::bad_alloc();
  }
  if (n == 0) n = 1;
  void* p = align <= alignof(std::max_align_t) ? std::malloc(n) : std::aligned_alloc(align, (n + align - 1) / align * align);
  if (!p && !nothrow) throw std::bad_alloc();
  return p;
}
void* operator new(std::size_t n) { return allocate(n, 0, false); }
void* operator new[](std::size_t n) { return allocate(n, 0, false); }
void* operator new(std::size_t n, std::align_val_t a) { return allocate(n, std::size_t(a), false); }
void* operator new[](std::size_t n, std::align_val_t a) { return allocate(n, std::size_t(a), false); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return allocate(n, 0, true); }
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept { return allocate(n, 0, true); }
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { return allocate(n, std::size_t(a), true); }
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { return allocate(n, std::size_t(a), true); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

#define EXPECT(cond)                                                              \
  do {                                                                            \
    if (!(cond)) {                                                                \
      dprintf(2, "k=%ld line %d: %s\n", k, __LINE__, #cond);                      \
      _exit(1);                                                                   \
    }                                                                             \
  } while (0)

// Nodes live in static arrays (no allocation by the test); a deleter marks them.
constexpr int N = 16;
struct RNode;
struct RMark {
  void operator()(RNode* p) const noexcept;
};
struct RNode : std::rcu_obj_base<RNode, RMark> {
  int deletions = 0;
};
void RMark::operator()(RNode* p) const noexcept { ++p->deletions; }
static RNode rnodes[N];
static int plain[N];
static int plain_deletions[N];
struct PlainMark {
  void operator()(int* p) const noexcept { ++plain_deletions[p - plain]; }
};

struct HNode;
struct HMark {
  void operator()(HNode* p) const noexcept;
};
struct HNode : std::hazard_pointer_obj_base<HNode, HMark> {
  std::atomic<int> deletions{0};
};
void HMark::operator()(HNode* p) const noexcept { p->deletions.fetch_add(1); }
static HNode hnodes[N];

static void scenario_rcu(long k) {
  fail_from = k;
  std::rcu_domain& d = std::rcu_default_domain();
  d.lock();
  const bool nested = d.try_lock();
  d.unlock();
  rnodes[0].retire();  // retired inside a region: deleted only after it ends
  d.unlock();
  {
    std::scoped_lock<std::rcu_domain> region(std::rcu_default_domain());
  }
  for (int i = 1; i < N; ++i) rnodes[i].retire(RMark{}, d);
  bool scheduled[N] = {};
  for (int i = 0; i < N; ++i) {
    try {
      std::rcu_retire(&plain[i], PlainMark{});
      scheduled[i] = true;
    } catch (const std::bad_alloc&) {
    }
  }
  std::rcu_synchronize();
  std::rcu_barrier();
  const bool any_failed = failed;
  fail_from = 0;
  EXPECT(&d == &std::rcu_default_domain());
  EXPECT(nested);
  for (int i = 0; i < N; ++i) EXPECT(rnodes[i].deletions == 1);
  for (int i = 0; i < N; ++i) EXPECT(plain_deletions[i] == (scheduled[i] ? 1 : 0));
  std::rcu_barrier();
  for (int i = 0; i < N; ++i) EXPECT(rnodes[i].deletions == 1);
  _exit(any_failed ? 0 : 10);
}

static void scenario_hazard(long k) {
  std::hazard_pointer h = std::make_hazard_pointer();  // (may throw: made before the failures)
  std::hazard_pointer h2 = std::make_hazard_pointer();
  std::atomic<HNode*> src{&hnodes[0]};
  fail_from = k;
  HNode* p = h.protect(src);
  HNode* q = &hnodes[1];
  std::atomic<HNode*> src2{q};
  const bool tp = h2.try_protect(q, src2);
  for (int i = 2; i < N; ++i) hnodes[i].retire();  // unprotected: may be reclaimed at once
  hnodes[0].retire();                               // protected by h since before
  hnodes[1].retire();                               // protected by h2 since before
  const int protected_deletions = hnodes[0].deletions.load() + hnodes[1].deletions.load();
  h2.reset_protection(q);
  h.reset_protection();
  h.swap(h2);
  const bool any_failed = failed;
  fail_from = 0;
  EXPECT(p == &hnodes[0] && tp && q == &hnodes[1]);
  EXPECT(protected_deletions == 0);
  for (int i = 0; i < N; ++i) EXPECT(hnodes[i].deletions.load() <= 1);
  _exit(any_failed ? 0 : 10);
}

static void sweep(const char* name, void (*scenario)(long)) {
  for (long k = 1; k <= 2000; ++k) {
    const pid_t pid = fork();
    CHECK(pid >= 0);
    if (pid == 0) scenario(k);
    int status = 0;
    CHECK(waitpid(pid, &status, 0) == pid);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 10) return;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      dprintf(2, "%s: k=%ld: %s %d\n", name, k, WIFEXITED(status) ? "exit" : "signal",
              WIFEXITED(status) ? WEXITSTATUS(status) : WTERMSIG(status));
      abort();
    }
  }
  dprintf(2, "%s: never completed\n", name);
  abort();
}

int main() {
  sweep("rcu", scenario_rcu);
  sweep("hazard pointers", scenario_hazard);
  return 0;
}
