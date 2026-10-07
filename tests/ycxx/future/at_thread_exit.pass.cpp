// [futures.promise]/23: set_value_at_thread_exit "Stores the value r in the shared state
// without making that state ready immediately. Schedules that state to be made ready when the
// current thread exits, after all objects with thread storage duration associated with the
// current thread have been destroyed." set_exception_at_thread_exit likewise.
// [futures.task.members]/25: make_ready_at_thread_exit for packaged_task.
// The thread_local probes are constructed before the action is scheduled, and their destructors
// check that the state is not ready yet: the order is checked where it happens, not by a later
// observation that a slow wake-up of the waiting thread could make pass.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <future>
#include <thread>
#include <atomic>
#include "check.hpp"

static std::atomic<int> tls_destroyed(0);
static bool (*state_ready)() = nullptr; // the state of the current round; set before its thread starts

template <class F>
bool is_ready(F& f) {
  return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

struct Probe {
  ~Probe() {
    CHECK(!state_ready());
    tls_destroyed.fetch_add(1);
  }
};
thread_local Probe probe;

static std::future<int>* fv;
static std::future<void>* fe;
static std::future<int>* ft;

int main() {
  std::promise<int> p;
  std::future<int> f = p.get_future();
  fv = &f;
  state_ready = [] { return is_ready(*fv); };
  std::atomic<bool> release(false);
  std::thread t([&] {
    (void)&probe;
    p.set_value_at_thread_exit(11);
    release.wait(false);  // the state stays not ready while the thread runs
  });
  CHECK(f.wait_for(std::chrono::milliseconds(2)) == std::future_status::timeout);
  release = true;
  release.notify_one();
  CHECK(f.get() == 11);
  CHECK(tls_destroyed.load() == 1);
  t.join();

  std::promise<void> pe;
  auto fe0 = pe.get_future();
  fe = &fe0;
  state_ready = [] { return is_ready(*fe); };
  std::thread t2([&] { (void)&probe; pe.set_exception_at_thread_exit(std::make_exception_ptr(3)); });
  bool caught = false;
  try { fe0.get(); } catch (int v) { caught = v == 3; }
  CHECK(caught && tls_destroyed.load() == 2);
  t2.join();

  std::packaged_task<int(int)> task([](int x) { return x + 1; });
  auto ft0 = task.get_future();
  ft = &ft0;
  state_ready = [] { return is_ready(*ft); };
  std::thread t3([&] { (void)&probe; task.make_ready_at_thread_exit(4); });
  CHECK(ft0.get() == 5);
  CHECK(tls_destroyed.load() == 3);
  t3.join();
  return 0;
}
