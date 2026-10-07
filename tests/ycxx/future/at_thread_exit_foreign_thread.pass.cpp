// The thread-end actions of a thread the program starts itself (pthread_create, not std::thread)
// run when it ends, after its thread_local objects constructed after the first action was
// scheduled ([futures.promise]/23, [thread.condition.nonmember]/2-3). That much holds on every
// platform; for objects constructed before it, see DECISIONS §3 (Darwin with Clang: libycxx does
// not see them, so it orders the actions after them only on the threads it starts and on the
// main thread). The destructors check that the actions are still pending.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <pthread.h>
#include "check.hpp"

static std::future<int> f;
static std::mutex m;
static std::condition_variable cv;
static std::atomic<int> destroyed(0);

struct Probe {
  ~Probe() {
    CHECK(f.wait_for(std::chrono::seconds(0)) == std::future_status::timeout);
    bool unlocked = false;
    std::thread([&] {
      unlocked = m.try_lock();
      if (unlocked)
        m.unlock();
    }).join();
    CHECK(!unlocked);
    destroyed.fetch_add(1);
  }
};

static Probe& late() {
  thread_local Probe p;
  return p;
}
static Probe& later() {
  thread_local Probe p;
  return p;
}

static void* body(void* arg) {
  auto* p = static_cast<std::promise<int>*>(arg);
  p->set_value_at_thread_exit(5);
  late();
  std::unique_lock lk(m);
  std::notify_all_at_thread_exit(cv, std::move(lk));
  later();
  return nullptr;
}

int main() {
  std::promise<int> p;
  f = p.get_future();
  pthread_t t;
  CHECK(pthread_create(&t, nullptr, &body, &p) == 0);
  CHECK(f.get() == 5);
  CHECK(destroyed.load() == 2);
  CHECK(pthread_join(t, nullptr) == 0);
  std::lock_guard g(m); // released by the thread's notification
  return 0;
}
