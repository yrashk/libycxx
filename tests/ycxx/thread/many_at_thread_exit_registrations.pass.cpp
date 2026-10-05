// Many "at thread exit" registrations from one thread, and many thread_local objects with
// nontrivial destructors in many threads at once. The draft sets no limit on either.
//   [futures.promise]/23 set_value_at_thread_exit, [futures.task.members]/25
//     make_ready_at_thread_exit: the state is made ready "when the current thread exits, after
//     all objects with thread storage duration associated with the current thread have been
//     destroyed".
//   [thread.condition.nonmember]/2-3 notify_all_at_thread_exit(cond, lk): ownership of lk's lock
//     is transferred to internal storage; at thread exit, after the thread_local objects are
//     destroyed, "lk.unlock(); cond.notify_all();".
//   [basic.start.term]/2: thread_local objects are destroyed when the thread's initial function
//     returns; /4 (with [stmt.dcl]) in reverse order of construction.
// One thread makes 2000 registrations of each kind (2000 distinct mutexes stay locked until it
// exits); then 48 threads each construct 300 thread_local objects of distinct types and make 40
// registrations of each kind. When any of the states becomes ready or any of the mutexes can be
// locked, every thread_local object of that thread has been destroyed, in reverse order.
// FLAGS: -pthread
#include <atomic>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

constexpr int Kinds = 300;

thread_local std::vector<int>* tl_log = nullptr;  // where this thread's thread_local objects report
thread_local std::atomic<int>* tl_count = nullptr;

template<int N> struct TL {
  std::vector<int>* log = tl_log;
  std::atomic<int>* count = tl_count;
  ~TL() {
    log->push_back(N);
    count->fetch_add(1);
  }
};
template<int N> void touch() {
  thread_local TL<N> object;  // block scope: constructed when control passes here ([stmt.dcl]/3)
  (void)object;
}
template<int... N> void touch_all(std::integer_sequence<int, N...>) { (touch<N>(), ...); }

struct PerThread {
  std::vector<int> log;
  std::atomic<int> destroyed{0};
  std::vector<std::mutex> mutexes;
  std::vector<std::condition_variable> cvs;
  std::vector<std::promise<int>> promises;
  std::vector<std::packaged_task<int(int)>> tasks;
  std::vector<std::future<int>> futures;
  explicit PerThread(int n) : mutexes(static_cast<std::size_t>(n)), cvs(static_cast<std::size_t>(n)), promises(static_cast<std::size_t>(n)) {
    for (int i = 0; i < n; ++i) {
      futures.push_back(promises[static_cast<std::size_t>(i)].get_future());
      tasks.emplace_back([](int x) { return x * 2; });
      futures.push_back(tasks.back().get_future());
    }
  }
};

static void body(PerThread* p, bool with_tls) {
  tl_log = &p->log;
  tl_count = &p->destroyed;
  if (with_tls) touch_all(std::make_integer_sequence<int, Kinds>{});
  const int n = static_cast<int>(p->mutexes.size());
  for (int i = 0; i < n; ++i) {
    std::unique_lock<std::mutex> lk(p->mutexes[static_cast<std::size_t>(i)]);
    std::notify_all_at_thread_exit(p->cvs[static_cast<std::size_t>(i)], std::move(lk));
    p->promises[static_cast<std::size_t>(i)].set_value_at_thread_exit(i);
    p->tasks[static_cast<std::size_t>(i)].make_ready_at_thread_exit(i);
  }
}

// Waits for every registration of p's (exited or exiting) thread; each one finds all of the
// thread's thread_local objects destroyed.
static bool check_exit(PerThread& p, int expected_tls) {
  bool ok = true;
  const int n = static_cast<int>(p.mutexes.size());
  for (int i = 0; i < n; ++i) {
    std::future<int>& f1 = p.futures[2 * static_cast<std::size_t>(i)];
    std::future<int>& f2 = p.futures[2 * static_cast<std::size_t>(i) + 1];
    f1.wait();
    ok = ok && p.destroyed.load() == expected_tls;
    f2.wait();
    ok = ok && p.destroyed.load() == expected_tls;
    ok = ok && f1.get() == i && f2.get() == 2 * i;
    p.mutexes[static_cast<std::size_t>(i)].lock();  // released at the thread's exit
    ok = ok && p.destroyed.load() == expected_tls;
    p.mutexes[static_cast<std::size_t>(i)].unlock();
  }
  if (expected_tls) {
    ok = ok && p.log.size() == static_cast<std::size_t>(expected_tls);
    for (std::size_t k = 0; ok && k < p.log.size(); ++k) ok = p.log[k] == expected_tls - 1 - static_cast<int>(k);
  }
  return ok;
}

int main() {
  watchdog(30);
  {
    PerThread p(2000);
    std::thread t(body, &p, false);
    CHECK(check_exit(p, 0));
    t.join();
  }
  {
    PerThread p(2000);
    std::thread t(body, &p, true);
    CHECK(check_exit(p, Kinds));
    t.join();
  }
  constexpr int Threads = 48;
  std::vector<std::unique_ptr<PerThread>> ps;
  std::vector<std::thread> ts;
  for (int i = 0; i < Threads; ++i) ps.push_back(std::make_unique<PerThread>(40));
  for (int i = 0; i < Threads; ++i) ts.emplace_back(body, ps[static_cast<std::size_t>(i)].get(), true);
  for (int i = Threads - 1; i >= 0; --i) CHECK(check_exit(*ps[static_cast<std::size_t>(i)], Kinds));
  for (auto& t : ts) t.join();
  return 0;
}
