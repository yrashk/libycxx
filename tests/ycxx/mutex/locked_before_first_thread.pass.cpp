// A mutex locked while the program has a single thread is still owned once threads exist:
// [thread.mutex.requirements.mutex.general]/15 lock(): "Blocks the calling thread until
// ownership of the mutex can be obtained for the calling thread"; /22 try_lock() "Attempts to
// obtain ownership ... without blocking. If ownership is not obtained, there is no effect";
// unlock() "Releases the calling thread's ownership" and /6 "prior unlock() operations on the
// same object shall synchronize with" a successful lock, so the blocked thread wakes up.
// [thread.timedmutex.requirements]: try_lock_for times out while another thread owns it.
// [thread.mutex.recursive]/3: another thread obtains a recursive_mutex only after all levels
// are released. [thread.sharedmutex.requirements]: exclusive lock() blocks while any thread
// holds shared ownership; lock_shared() blocks while a thread holds exclusive ownership.
// [thread.condition.condvarany]: wait unlocks and relocks a user lock.
// Each mutex type runs in its own child process, so that the mutex is locked before that
// process's first thread starts.
// FLAGS: -pthread
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include "child_process.hpp"
#include "check.hpp"
#include "watchdog.hpp"

using namespace std::chrono_literals;

static std::atomic<int> stage{0};

static void settle() { std::this_thread::sleep_for(30ms); }

template <class M>
static void exclusive(int levels) {
  static M m;  // constructed before any thread
  for (int i = 0; i < levels; ++i) m.lock();
  std::thread t([] {
    if (m.try_lock()) stage = -1;  // must fail: main owns it
    if constexpr (requires { m.try_lock_for(1ms); }) {
      if (m.try_lock_for(20ms)) stage = -1;
    }
    stage = 1;
    m.lock();  // blocks until main released every level
    stage = 2;
    m.unlock();
  });
  while (stage.load() < 1) std::this_thread::yield();
  settle();
  CHECK(stage.load() == 1);  // still blocked
  for (int i = 0; i < levels - 1; ++i) {
    m.unlock();
    settle();
    CHECK(stage.load() == 1);  // a recursive mutex: still owned
  }
  m.unlock();
  t.join();
  CHECK(stage.load() == 2);
  // usable again from main, and contended both ways
  long counter = 0;
  std::thread u([&] {
    for (int i = 0; i < 20000; ++i) {
      std::lock_guard g(m);
      ++counter;
    }
  });
  for (int i = 0; i < 20000; ++i) {
    std::lock_guard g(m);
    ++counter;
  }
  u.join();
  CHECK(counter == 40000);
}

template <class M>
static void shared() {
  static M m;
  m.lock_shared();  // shared ownership before any thread
  std::thread t([] {
    m.lock_shared();  // other shared owners are fine
    m.unlock_shared();
    if (m.try_lock()) stage = -1;
    stage = 1;
    m.lock();  // blocks until main's shared ownership is released
    stage = 2;
    m.unlock();
  });
  while (stage.load() < 1) std::this_thread::yield();
  settle();
  CHECK(stage.load() == 1);
  m.unlock_shared();
  t.join();
  CHECK(stage.load() == 2);
  // and the other way: exclusive before, shared waiter
  stage = 0;
  m.lock();
  std::thread t2([] {
    if (m.try_lock_shared()) stage = -1;
    stage = 1;
    m.lock_shared();
    stage = 2;
    m.unlock_shared();
  });
  while (stage.load() < 1) std::this_thread::yield();
  settle();
  CHECK(stage.load() == 1);
  m.unlock();
  t2.join();
  CHECK(stage.load() == 2);
}

static void cv_any() {
  static std::mutex m;
  static std::condition_variable_any cv;
  static bool ready = false;
  std::unique_lock l(m);  // held before the thread starts
  std::thread t([] {
    std::unique_lock l2(m);  // blocks until main waits (which unlocks)
    ready = true;
    stage = 1;
    cv.notify_all();
  });
  cv.wait(l, [] { return ready; });
  CHECK(stage.load() == 1);
  l.unlock();
  t.join();
}

int main(int argc, char** argv) {
  (void)argc;
  if (const char* mode = child_mode()) {
    watchdog(30);
    std::string md = argv[1];
    (void)mode;
    if (md == "mutex") exclusive<std::mutex>(1);
    else if (md == "recursive") exclusive<std::recursive_mutex>(3);
    else if (md == "timed") exclusive<std::timed_mutex>(1);
    else if (md == "recursive_timed") exclusive<std::recursive_timed_mutex>(2);
    else if (md == "shared") shared<std::shared_mutex>();
    else if (md == "shared_timed") shared<std::shared_timed_mutex>();
    else if (md == "cv_any") cv_any();
    CHECK(stage.load() >= 0);
    return 0;
  }
  for (const char* md : {"mutex", "recursive", "timed", "recursive_timed", "shared", "shared_timed", "cv_any"}) {
    ChildResult r = run_self(md);
    if (r.status != 0) dprintf(2, "mode %s: status %d\n%s\n", md, r.status, r.err.c_str());
    CHECK(r.status == 0);
  }
}
