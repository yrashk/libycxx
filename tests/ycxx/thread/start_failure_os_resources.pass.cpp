// Starting a thread when the system cannot create one (the address space is limited, so a new
// thread's stack cannot be mapped), for thread, jthread and async(launch::async).
//   [thread.thread.constr]/9: "Throws: system_error if unable to start the new thread." /10:
//     "Error conditions: resource_unavailable_try_again — the system lacked the necessary
//     resources to create another thread". [res.on.exception.handling]: a failure to allocate
//     storage may be reported as bad_alloc instead.
//   /6: the decay-copies of f and the arguments are materialized in the constructing thread; when
//     the constructor throws no thread runs, so the callable is never invoked and every copy is
//     destroyed. Nothing the attempt allocated stays allocated (operator new/delete balance) and
//     the thread object is not joinable.
//   [thread.jthread.cons]/5-8: likewise for jthread (its stop state included).
//   [futures.async]/5-6: "Throws: system_error if policy == launch::async and the implementation
//     is unable to start a new thread, or std::bad_alloc if memory for the internal data
//     structures cannot be allocated." "Error conditions: resource_unavailable_try_again — if
//     policy == launch::async and the system is unable to start a new thread."
// Once threads are released, starting a thread succeeds again.
// Runs in a child process with RLIMIT_AS set just above the current size (Linux: the size comes
// from /proc/self/statm; where it is not available, or where the limit does not stop a thread
// from starting, the child reports that and the test checks nothing).
// UNSUPPORTED-SANITIZER: asan,tsan  the sanitizer runtimes reserve address space of their own and cannot run under a small RLIMIT_AS
// FLAGS: -pthread
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <new>
#include <system_error>
#include <thread>
#include <vector>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

static std::atomic<long> live_allocs{0};
void* operator new(std::size_t n) {
  void* p = std::malloc(n ? n : 1);
  if (!p) throw std::bad_alloc();
  live_allocs.fetch_add(1);
  return p;
}
void operator delete(void* p) noexcept {
  if (p) live_allocs.fetch_sub(1);
  std::free(p);
}
void operator delete(void* p, std::size_t) noexcept { operator delete(p); }

static std::atomic<int> arg_live{0}, invoked{0};
struct Arg {
  Arg() { arg_live.fetch_add(1); }
  Arg(const Arg&) { arg_live.fetch_add(1); }
  Arg(Arg&&) noexcept { arg_live.fetch_add(1); }
  ~Arg() { arg_live.fetch_sub(1); }
};
struct Fn {
  Arg held;
  void operator()(const Arg&) const { invoked.fetch_add(1); }
  void operator()(std::stop_token, const Arg&) const { invoked.fetch_add(1); }
};

static std::atomic<bool> release{false};
static void blocker() {
  while (!release.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

enum Outcome { ok_system_error = 0, ok_bad_alloc = 1, no_failure = 2, bad = 3 };

// Fills the address space with blocked threads until creating one fails, then checks one
// attempt of the given kind.
static int attempt(int kind) {
  std::vector<std::thread> blockers;
  blockers.reserve(20000);
  for (int i = 0; i < 20000; ++i) {
    try {
      blockers.emplace_back(blocker);
    } catch (...) {
      break;
    }
  }
  if (blockers.size() == 20000) {
    release.store(true);
    for (auto& t : blockers) t.join();
    return no_failure;
  }
  int result = bad;
  {
    const Fn fn;
    const Arg arg;
    const int args_before = arg_live.load();
    const long allocs_before = live_allocs.load();
    try {
      if (kind == 0) {
        std::thread t(fn, arg);
        t.join();
        result = no_failure;
      } else if (kind == 1) {
        std::jthread t(fn, arg);
        result = no_failure;
      } else {
        auto f = std::async(std::launch::async, fn, arg);
        f.get();
        result = no_failure;
      }
    } catch (const std::system_error& e) {
      result = e.code() == std::errc::resource_unavailable_try_again ? ok_system_error : bad;
      if (result == bad) dprintf(2, "kind %d: system_error %s\n", kind, e.what());
    } catch (const std::bad_alloc&) {
      result = ok_bad_alloc;
    }
    if (result != no_failure) {
      if (invoked.load() != 0 || arg_live.load() != args_before || live_allocs.load() != allocs_before) {
        dprintf(2, "kind %d: invoked %d, copies left %d, allocations left %ld\n", kind, invoked.load(),
                arg_live.load() - args_before, live_allocs.load() - allocs_before);
        result = bad;
      }
    }
  }
  release.store(true);
  for (auto& t : blockers) t.join();
  // Resources are back: a thread starts.
  if (result != bad) {
    std::thread t([] { invoked.fetch_add(100); });
    t.join();
    if (invoked.load() != 100 && result != no_failure) result = bad;
  }
  return result;
}

static int child(int kind) {
  FILE* f = std::fopen("/proc/self/statm", "r");
  if (!f) return no_failure;
  unsigned long pages = 0;
  const int got = std::fscanf(f, "%lu", &pages);
  std::fclose(f);
  if (got != 1) return no_failure;
  const unsigned long size = pages * static_cast<unsigned long>(sysconf(_SC_PAGESIZE));
  rlimit lim{};
  lim.rlim_cur = lim.rlim_max = size + (48ul << 20);
  if (setrlimit(RLIMIT_AS, &lim) != 0) return no_failure;
  return attempt(kind);
}

int main() {
  for (int kind = 0; kind < 3; ++kind) {
    std::fflush(nullptr);
    const pid_t pid = fork();
    CHECK(pid >= 0);
    if (pid == 0) _exit(child(kind));
    int status = 0;
    CHECK(waitpid(pid, &status, 0) == pid);
    CHECK(WIFEXITED(status));
    const int r = WEXITSTATUS(status);
    if (r == no_failure) dprintf(2, "kind %d: the limit did not stop threads from starting (nothing checked)\n", kind);
    if (r == ok_bad_alloc) dprintf(2, "kind %d: reported as bad_alloc\n", kind);
    CHECK(r != bad);
  }
  return 0;
}
