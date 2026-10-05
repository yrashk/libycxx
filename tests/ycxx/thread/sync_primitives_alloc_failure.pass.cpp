// Blocking synchronization between threads (latch, barrier, counting_semaphore, mutex,
// condition_variable, shared_mutex) while operator new fails: every call from the k-th on
// fails, for k = 1, 2, ... until the scenario makes no allocation that fails. Each run is a
// fresh child process whose threads are created before the failures start, so the first
// waits and wake-ups of the process happen during them.
//   [thread.latch.class]/9-12: count_down, wait, arrive_and_wait "Throws: system_error when an
//     exception is required ([thread.req.exception])", try_wait noexcept;
//     [thread.barrier.class]: arrive, wait, arrive_and_wait: system_error;
//     [thread.sema.cnt]/11: release: system_error; try_acquire noexcept; try_acquire_for:
//     timeout-related exceptions or system_error; [thread.mutex.requirements.mutex.general]
//     lock(): system_error, unlock(): "Throws: Nothing"; [thread.sharedmutex.requirements]
//     likewise; [thread.condition.condvar]: notify_one/notify_all noexcept, wait(lock):
//     "Throws: Nothing", wait(lock, pred): "Any exception thrown by pred" (none here), so the
//     condition variable waits must not throw at all.
//   [res.on.exception.handling]/1: a function reports a failure only with an exception of a
//     type in its Throws: element: here system_error (or a type derived from it). A bad_alloc
//     escaping any of these calls is a failure of the test; a system_error is accepted (the
//     child then stops: the protocol between the threads cannot go on).
// Every operation that completes must have its specified effect: the latch and the barrier
// release every thread, the completion function runs once per phase, the semaphore hands over
// exactly as many units as released, the counter protected by the mutexes is exact.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <atomic>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <latch>
#include <mutex>
#include <new>
#include <semaphore>
#include <shared_mutex>
#include <system_error>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

static long fail_from = 0, calls = 0;
static bool failed = false;
static void* allocate(std::size_t n, std::size_t align, bool nothrow) {
  if (__atomic_load_n(&fail_from, __ATOMIC_RELAXED) && __atomic_add_fetch(&calls, 1, __ATOMIC_RELAXED) >= __atomic_load_n(&fail_from, __ATOMIC_RELAXED)) {
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

constexpr int T = 4, rounds = 50;

// Runs body(i) in T threads; exceptions: system_error ends the child with status 20 (accepted),
// anything else with 1.
template <class F>
static void guarded(F& body, int i) {
  try {
    body(i);
  } catch (const std::system_error&) {
    _exit(20);
  } catch (const std::bad_alloc&) {
    dprintf(2, "thread %d: bad_alloc escaped\n", i);
    _exit(1);
  } catch (...) {
    dprintf(2, "thread %d: another exception escaped\n", i);
    _exit(1);
  }
}

static void scenario(long k) {
  alarm(30);  // a lost wake-up would block forever
  std::latch start(T + 1), done(T);
  int completions = 0;
  auto on_completion = [&]() noexcept { ++completions; };
  std::barrier<decltype(on_completion)> phase(T, on_completion);
  std::counting_semaphore<> tokens(0);
  std::atomic<int> taken{0};
  std::mutex m;
  std::condition_variable cv;
  std::shared_mutex sm;
  long counter = 0, exclusive_counter = 0, shared_reads = 0;
  int turn = 0;
  auto body = [&](int i) {
    start.arrive_and_wait();
    for (int r = 0; r < rounds; ++r) {
      {
        std::lock_guard g(m);
        ++counter;
      }
      {
        std::unique_lock l(m);
        try {
          cv.wait(l, [&] { return turn % T == i; });
        } catch (...) {
          dprintf(2, "thread %d: condition_variable::wait threw\n", i);
          _exit(1);
        }
        ++turn;
        cv.notify_all();
      }
      {
        std::shared_lock s(sm);
        __atomic_add_fetch(&shared_reads, 1, __ATOMIC_RELAXED);
      }
      {
        std::unique_lock x(sm);
        ++exclusive_counter;
      }
      tokens.release();
      if (tokens.try_acquire_for(std::chrono::seconds(10))) taken.fetch_add(1);
      phase.arrive_and_wait();
    }
    done.count_down();
  };
  auto run = [&](int i) { guarded(body, i); };
  std::thread ts[T];
  for (int i = 0; i < T; ++i) ts[i] = std::thread(run, i);  // (before the failures)
  __atomic_store_n(&fail_from, k, __ATOMIC_RELAXED);
  try {
    start.count_down();
    done.wait();
  } catch (const std::system_error&) {
    _exit(20);
  }
  const bool any_failed = __atomic_load_n(&failed, __ATOMIC_RELAXED);
  __atomic_store_n(&fail_from, 0L, __ATOMIC_RELAXED);
  for (auto& t : ts) t.join();
  bool ok = true;
  ok = ok && counter == long(T) * rounds && exclusive_counter == long(T) * rounds && turn == T * rounds && shared_reads == long(T) * rounds;
  ok = ok && completions == rounds && taken.load() == T * rounds && !tokens.try_acquire();
  ok = ok && start.try_wait() && done.try_wait();
  if (!ok)
    dprintf(2, "k=%ld: counter %ld turn %d reads %ld completions %d taken %d\n", k, counter, turn, shared_reads,
            completions, taken.load());
  _exit(!ok ? 1 : any_failed ? 0 : 10);
}

int main() {
  long accepted_system_errors = 0;
  for (long k = 1;; ++k) {
    CHECK(k <= 3000);
    const pid_t pid = fork();
    CHECK(pid >= 0);
    if (pid == 0) scenario(k);
    int status = 0;
    CHECK(waitpid(pid, &status, 0) == pid);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 10) break;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 20) {
      ++accepted_system_errors;
      continue;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      dprintf(2, "k=%ld: %s %d\n", k, WIFEXITED(status) ? "exit" : "signal", WIFEXITED(status) ? WEXITSTATUS(status) : WTERMSIG(status));
      CHECK(false);
    }
  }
  if (accepted_system_errors) dprintf(1, "%ld runs reported system_error\n", accepted_system_errors);
  return 0;
}
