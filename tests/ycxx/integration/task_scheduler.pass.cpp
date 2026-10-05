// Whole-program integration: a single-worker priority task scheduler.
//   jthread ([thread.jthread.class]): the worker gets a stop_token; the destructor requests stop
//     and joins ([thread.jthread.cons]/7).
//   condition_variable_any interruptible waits ([thread.condvarany.intwait]/1-2, /7): "will be
//     notified when there is a stop request on the passed stop_token. In that case the functions
//     return immediately, returning false if the predicate evaluates to false"; wait_until with
//     a stop_token returns on a stop request long before abs_time.
//   priority_queue ([priority.queue]) with a comparator: the largest element (here: highest
//     priority, then lowest sequence number) is top(); tasks submitted by a running task are
//     ordered with the others.
//   stop_callback ([stopcallback.cons]/2-3): runs in the thread that calls request_stop, or
//     immediately in the constructing thread when stop was already requested.
//   exception_ptr ([propagation]/7-10): an exception escaping a task is carried to the main
//     thread and rethrown there.
//   chrono: steady_clock is monotonic ([time.clock.steady]); "{}" of durations
//     ([time.format]/... operator<< suffixes: [time.duration.io]/1).
// FLAGS: -pthread
// REQUIRES: exceptions
#include <chrono>
#include <condition_variable>
#include <exception>
#include <format>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>
#include "watchdog.hpp"
#include "check.hpp"

using namespace std::chrono_literals;

class Scheduler {
public:
  struct Task {
    int priority;
    int seq;
    std::string name;
    std::function<void()> fn;
  };
  struct Order {
    bool operator()(const Task& a, const Task& b) const {
      return a.priority != b.priority ? a.priority < b.priority : a.seq > b.seq;
    }
  };

  Scheduler() : worker_([this](std::stop_token st) { run(st); }) {}

  void submit(int priority, std::string name, std::function<void()> fn = [] {}) {
    {
      std::lock_guard l(m_);
      q_.push(Task{priority, seq_++, std::move(name), std::move(fn)});
    }
    cv_.notify_all();
  }
  void resume() {
    {
      std::lock_guard l(m_);
      paused_ = false;
    }
    cv_.notify_all();
  }
  std::vector<std::string> wait_done(std::size_t n) {
    std::unique_lock l(m_);
    cv_.wait(l, [&] { return done_.size() >= n; });
    return done_;
  }
  std::exception_ptr error() {
    std::lock_guard l(m_);
    return error_;
  }
  std::stop_source stop_source() { return worker_.get_stop_source(); }
  bool exited() {
    std::lock_guard l(m_);
    return exited_;
  }

private:
  void run(std::stop_token st) {
    std::unique_lock lk(m_);
    while (cv_.wait(lk, st, [&] { return !paused_ && !q_.empty(); })) {
      Task t = q_.top();
      q_.pop();
      lk.unlock();
      try {
        t.fn();
      } catch (...) {
        std::lock_guard l(m_);
        error_ = std::current_exception();
      }
      lk.lock();
      done_.push_back(std::format("{}@{}", t.name, t.priority));
      cv_.notify_all();
    }
    exited_ = true;
  }

  std::mutex m_;
  std::condition_variable_any cv_;
  std::priority_queue<Task, std::vector<Task>, Order> q_;
  bool paused_ = true, exited_ = false;
  int seq_ = 0;
  std::vector<std::string> done_;
  std::exception_ptr error_;
  std::jthread worker_;  // last: constructed after, destroyed (stopped and joined) before the rest
};

int main() {
  watchdog(30);
  std::thread::id worker_id;
  {
    Scheduler s;
    s.submit(1, "A");
    s.submit(5, "B", [&] {
      worker_id = std::this_thread::get_id();
      s.submit(10, "F");  // runs before C (higher priority)
      s.submit(0, "G");
    });
    s.submit(5, "C");
    s.submit(3, "D", [] { throw std::runtime_error("D failed"); });
    s.submit(1, "E");
    auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(20ms);  // paused: nothing runs
    CHECK(s.wait_done(0).empty());
    s.resume();
    auto done = s.wait_done(7);
    CHECK((done == std::vector<std::string>{"B@5", "F@10", "C@5", "D@3", "A@1", "E@1", "G@0"}));
    CHECK(std::chrono::steady_clock::now() - t0 >= 20ms);
    CHECK(worker_id != std::this_thread::get_id());

    bool rethrown = false;
    try {
      std::rethrow_exception(s.error());
    } catch (const std::runtime_error& e) {
      rethrown = std::string(e.what()) == "D failed";
    }
    CHECK(rethrown);

    // A stop callback registered on the worker's token runs in the thread requesting stop.
    std::stop_source src = s.stop_source();
    std::thread::id cb_thread;
    std::stop_callback cb(src.get_token(), [&] { cb_thread = std::this_thread::get_id(); });
    CHECK(!s.exited());
    CHECK(src.request_stop());
    CHECK(cb_thread == std::this_thread::get_id());
    // The worker, blocked in the interruptible wait with an empty queue, returns false and exits.
    while (!s.exited()) std::this_thread::yield();
    // Already stopped: a new callback runs immediately, in this thread.
    bool immediate = false;
    std::stop_callback cb2(src.get_token(), [&] { immediate = true; });
    CHECK(immediate);
  }

  // wait_until with a stop_token: a stop request ends a one-hour wait promptly.
  {
    std::mutex m;
    std::condition_variable_any cv;
    bool result = true;
    std::chrono::steady_clock::duration waited{};
    std::jthread t([&](std::stop_token st) {
      std::unique_lock lk(m);
      auto start = std::chrono::steady_clock::now();
      result = cv.wait_until(lk, st, start + 1h, [] { return false; });
      waited = std::chrono::steady_clock::now() - start;
    });
    std::this_thread::sleep_for(10ms);
    t.request_stop();
    t.join();
    CHECK(!result);
    CHECK(waited < 20s);
  }
  CHECK(std::format("{} {} {}", 20ms, std::chrono::duration<double>(1.5), 3h) == "20ms 1.5s 3h");
  return 0;
}
