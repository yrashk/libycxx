// Thread-storage objects of library types and the "at thread exit" facilities.
//   [basic.start.term]/2: "Constructed complete objects with thread storage duration within a
//     given thread are destroyed as a result of returning from the initial function of that
//     thread"; /4: in reverse order of construction.
//   [futures.promise]/23 set_value_at_thread_exit (and set_exception_at_thread_exit):
//     "Schedules that state to be made ready when the current thread exits, after all objects
//     with thread storage duration associated with the current thread have been destroyed."
//   [futures.task.members]/25 make_ready_at_thread_exit: likewise.
//   [thread.condition.nonmember]/2-3 notify_all_at_thread_exit: the notification and the
//     implied lk.unlock() are sequenced after the destruction of all objects with thread storage
//     duration associated with the current thread.
// "All" includes objects first constructed AFTER the scheduling call (B below), and objects
// constructed before it (A): neither order of registration may make the state ready early. Each
// destructor also asks whether the scheduled action has already happened (action_done) and logs
// "early" if so: the order is checked where it happens, not only by what the waiting thread
// observes once it wakes up.
//   [thread.thread.member]/4: the completion of the thread synchronizes with join()'s return;
//     the thread completes after returning from its initial function, i.e. after its
//     thread-storage objects are destroyed ([basic.start.term]/2).
//   [futures.async]/4.2 (launch::async: "as if in a new thread of execution"): the associated
//     thread completion synchronizes with the first function that detects the ready state.
// Each thread-storage object holds library types (string, map, unordered_map, shared_ptr); its
// destructor appends to a log protected by a mutex the waiting thread never holds.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "check.hpp"

static std::mutex log_m;
static std::vector<std::string> log_v;

static void log(const std::string& s) {
  std::lock_guard l(log_m);
  log_v.push_back(s);
}
static std::vector<std::string> take_log() {
  std::lock_guard l(log_m);
  auto v = std::move(log_v);
  log_v.clear();
  return v;
}

// Whether the action scheduled by the current section has happened; null where none is scheduled.
// Set before the section's thread starts.
static std::atomic<bool (*)()> action_done{nullptr};
template <class F>
static bool is_ready(F& f) {
  return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
static std::future<int>* f_int;
static std::future<void>* f_void;
static std::future<std::string>* f_string;
static std::mutex* notify_m;

struct TL {
  std::string name;
  std::map<int, std::string> m{{1, "one"}};
  std::unordered_map<std::string, int> u{{"k", 1}};
  std::shared_ptr<int> sp;
  explicit TL(std::string n) : name(std::move(n)), sp(std::make_shared<int>(7)) {}
  ~TL() {
    if (bool (*done)() = action_done.load(); done != nullptr && done())
      log("early " + name);
    log("dtor " + name + " " + m.at(1) + std::to_string(u.at("k")) + std::to_string(*sp));
  }
};
static TL& tl_a() {
  thread_local TL a("A");
  return a;
}
static TL& tl_b() {
  thread_local TL b("B");
  return b;
}

static const std::vector<std::string> both = {"dtor B one17", "dtor A one17"};

int main() {
  // set_value_at_thread_exit, in a detached thread: the future is the only synchronization.
  {
    std::promise<int> p;
    std::future<int> f = p.get_future();
    f_int = &f;
    action_done = [] { return is_ready(*f_int); };
    std::thread([p = std::move(p)]() mutable {
      tl_a();
      p.set_value_at_thread_exit(42);
      tl_b();
      CHECK(take_log().empty());
    }).detach();
    CHECK(f.get() == 42);
    CHECK(take_log() == both);
  }
  // set_exception_at_thread_exit.
  {
    std::promise<void> p;
    std::future<void> f = p.get_future();
    f_void = &f;
    action_done = [] { return is_ready(*f_void); };
    std::thread([p = std::move(p)]() mutable {
      tl_a();
      p.set_exception_at_thread_exit(std::make_exception_ptr(std::string("boom")));
      tl_b();
    }).detach();
    bool caught = false;
    try {
      f.get();
    } catch (const std::string& s) {
      caught = s == "boom";
    }
    CHECK(caught);
    CHECK(take_log() == both);
  }
  // packaged_task::make_ready_at_thread_exit; the task itself constructs B.
  {
    std::packaged_task<std::string(int)> task([](int n) {
      tl_b().m[2] = "two";
      return std::to_string(n) + tl_b().m.at(2);
    });
    std::future<std::string> f = task.get_future();
    f_string = &f;
    action_done = [] { return is_ready(*f_string); };
    std::thread([task = std::move(task)]() mutable {
      tl_a();
      task.make_ready_at_thread_exit(5);
    }).detach();
    CHECK(f.get() == "5two");
    CHECK(take_log() == both);
  }
  // notify_all_at_thread_exit.
  {
    std::mutex m;
    std::condition_variable cv;
    bool done = false;
    notify_m = &m;
    // The lock is held by the exiting thread until the notification: another thread's try_lock
    // fails. (The exiting thread may not try it: [thread.mutex.requirements.mutex.general].)
    action_done = [] {
      bool unlocked = false;
      std::thread([&] {
        unlocked = notify_m->try_lock();
        if (unlocked)
          notify_m->unlock();
      }).join();
      return unlocked;
    };
    std::thread([&] {
      tl_a();
      std::unique_lock lk(m);
      done = true;
      std::notify_all_at_thread_exit(cv, std::move(lk));
      tl_b();
    }).detach();
    std::unique_lock lk(m);
    cv.wait(lk, [&] { return done; });
    CHECK(take_log() == both);
  }
  action_done = nullptr;
  // join.
  {
    std::weak_ptr<int> w;
    std::thread t([&] {
      tl_a();
      w = tl_b().sp;
    });
    t.join();
    CHECK(take_log() == both);
    CHECK(w.expired());
  }
  // jthread (its destructor joins).
  {
    { std::jthread t([] { tl_b(); tl_a(); }); }
    CHECK((take_log() == std::vector<std::string>{"dtor A one17", "dtor B one17"}));
  }
  // async with launch::async.
  {
    auto f = std::async(std::launch::async, [] {
      tl_a();
      tl_b();
      return 1;
    });
    CHECK(f.get() == 1);
    CHECK(take_log() == both);
  }
  // The main thread's objects are untouched by all this.
  CHECK(tl_a().name == "A" && take_log().empty());
  return 0;
}
