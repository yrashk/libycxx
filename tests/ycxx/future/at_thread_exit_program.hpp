// Shared by the at_thread_exit_by_*.pass.cpp programs: the thread that ends the program registers
// every kind of thread-exit action, and the program checks, while it terminates, that they ran
// after that thread's thread_local objects were destroyed and before the static objects were
// destroyed and the atexit functions called.
//
// [futures.promise]/23, /26: set_value_at_thread_exit / set_exception_at_thread_exit "Schedules
// that state to be made ready when the current thread exits, after all objects with thread storage
// duration associated with the current thread have been destroyed."
// [futures.task.members]: make_ready_at_thread_exit, likewise.
// [thread.condition.nonmember]/2-3: notify_all_at_thread_exit: "This notification is sequenced
// after all objects with thread storage duration associated with the current thread have been
// destroyed and is equivalent to: cond.notify_all(); lk.unlock();"
// [support.start.term]/9.1: exit(): "First, objects with thread storage duration and associated
// with the current thread are destroyed. Next, objects with static storage duration are destroyed
// and functions registered by calling atexit are called." [basic.start.main]/5: a return from
// main calls exit. [basic.start.term]/2: thread_local objects are destroyed "as a result of that
// thread calling std::exit"; their destruction strongly happens before destroying any object with
// static storage duration.
#pragma once
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <future>
#include <mutex>

namespace ate {

// Destroyed after the checker (constructed before it).
inline std::mutex m;
inline std::condition_variable cv;
inline std::future<int> fv;
inline std::future<void> fe;
inline std::future<long> ft;
inline bool tls_saw_ready = false; // a thread_local destructor saw an action done
inline int tls_destroyed = 0;
inline bool checked = false;
// Never destroyed (no promise is abandoned); globals, so that a leak checker sees them reachable.
inline std::promise<int>* pv;
inline std::promise<void>* pe;
inline std::packaged_task<long(long)>* task;

inline bool ready(auto& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

[[noreturn]] inline void fail(const char* what) {
  std::fprintf(stderr, "FAIL: %s\n", what);
  std::fflush(stderr);
  std::_Exit(3);
}

// True when the lock that notify_all_at_thread_exit took over has been released.
inline bool unlocked() {
  if (!m.try_lock())
    return false;
  m.unlock();
  return true;
}

// Run once, by the atexit function: get() leaves the futures without a state.
inline void check_done() {
  if (tls_destroyed != 2)
    fail("the exiting thread's thread_local objects were not destroyed first");
  if (tls_saw_ready)
    fail("an action ran before a thread_local object of its thread was destroyed");
  if (!ready(fv) || fv.get() != 42)
    fail("set_value_at_thread_exit: not ready");
  if (!ready(fe))
    fail("set_exception_at_thread_exit: not ready");
  try {
    fe.get();
    fail("set_exception_at_thread_exit: no exception");
  } catch (int v) {
    if (v != 7)
      fail("set_exception_at_thread_exit: wrong exception");
  }
  if (!ready(ft) || ft.get() != 11)
    fail("make_ready_at_thread_exit: not ready");
  if (!unlocked())
    fail("notify_all_at_thread_exit: the lock was not released");
  checked = true;
  std::fprintf(stderr, "thread-exit actions done before the atexit functions\n");
}

// The last static object constructed before main, so the first destroyed.
struct checker {
  ~checker() {
    if (!checked)
      fail("the atexit function did not run before the static objects were destroyed");
    std::fprintf(stderr, "static destruction after the thread-exit actions\n");
  }
};
inline checker the_checker;

struct probe {
  ~probe() {
    // Every action is still pending while the thread's thread_local objects are destroyed. (The
    // mutex is not tried here: this thread may own it: [thread.mutex.requirements.mutex.general].)
    if (ready(fv) || ready(fe) || ready(ft))
      tls_saw_ready = true;
    ++tls_destroyed;
  }
};
inline thread_local probe before; // constructed before the actions are registered
inline thread_local probe after;  // and after

// Called on the thread that will end the program.
inline void register_actions() {
  static_cast<void>(&before);
  // The promises and the task live on the heap, never destroyed: the shared states stay alive
  // through the futures, and no promise is abandoned at the end of a scope.
  pv = new std::promise<int>;
  fv = pv->get_future();
  pv->set_value_at_thread_exit(42);
  pe = new std::promise<void>;
  fe = pe->get_future();
  pe->set_exception_at_thread_exit(std::make_exception_ptr(7));
  task = new std::packaged_task<long(long)>([](long x) { return x + 1; });
  ft = task->get_future();
  task->make_ready_at_thread_exit(10);
  std::unique_lock lk(m);
  std::notify_all_at_thread_exit(cv, std::move(lk));
  static_cast<void>(&after);
  if (ready(fv) || ready(fe) || ready(ft))
    fail("an action ran before the thread exited");
  // atexit functions registered now run before the static objects constructed earlier are
  // destroyed, and after the thread's actions.
  std::atexit([] { check_done(); });
}

} // namespace ate
