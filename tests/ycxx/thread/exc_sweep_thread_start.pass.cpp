// Starting threads and creating the synchronisation objects that own heap state while operator
// new fails at its k-th call, for every k until the operation completes.
//   [thread.thread.constr]/5-9: thread(F&& f, Args&&... args): the values produced by auto(...)
//     are "materialized in the constructing thread"; "Throws: system_error if unable to start
//     the new thread" ([res.on.exception.handling]: an allocation failure as bad_alloc). When
//     the constructor throws, no thread has been started: the callable never runs, and the
//     decay-copies made for it are destroyed (counted below); nothing is leaked.
//   [thread.jthread.cons]/4-7: likewise for jthread (with its stop_source).
//   [futures.async]/5: "Throws: system_error if policy == launch::async and the implementation
//     is unable to start a new thread, or std::bad_alloc if memory for the internal data
//     structures cannot be allocated." For launch::deferred, the function is not run.
//   [thread.condition.condvarany]/?: condition_variable_any(): "Throws: bad_alloc if memory
//     cannot be allocated for the internal state." [stopsource.cons]/1-3: stop_source():
//     "Throws: bad_alloc if memory cannot be allocated for the stop state."
//   [thread.condition.nonmember] notify_all_at_thread_exit, [futures.promise]
//     set_value_at_thread_exit: when the registration fails (the call throws), the lock is
//     not kept and the state is not made ready at thread exit (no hang either way).
// FLAGS: -pthread
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <stop_token>
#include <system_error>
#include <thread>
#include "exc_new.hpp"

using namespace exh;

static std::atomic<int> live_args{0}, runs{0};
struct Arg {
  int v;
  explicit Arg(int x) : v(x) { ++live_args; }
  Arg(const Arg& o) : v(o.v) { ++live_args; }
  Arg(Arg&& o) noexcept : v(o.v) { ++live_args; }
  ~Arg() { --live_args; }
};
struct Fn {
  Arg a{1};
  void operator()(const Arg& b) const { runs += a.v + b.v; }
  void operator()(std::stop_token, const Arg& b) const { runs += a.v + b.v; }
};

template <class F>
void sw(const char* name, F op) {
  sweep_new(name, [&] {
    runs = 0;
    bool wrong_type = false;
    bool threw = attempt([&] {
      try {
        op();
      } catch (const std::bad_alloc&) {
        throw;
      } catch (const std::system_error&) {
        disarm();
        throw alloc_failure(gnew);
      } catch (...) {
        wrong_type = true;
      }
    });
    disarm();
    EXH_EXPECT(!wrong_type, "an exception neither bad_alloc nor system_error");
    EXH_EXPECT(threw ? runs == 0 : runs == 3, "the callable ran although the start failed (or did not run)");
    EXH_EXPECT(live_args == 0, "a decay-copy of the callable or an argument was not destroyed");
    return threw;
  });
}

int main() {
  sw("thread", [] {
    Fn f;
    Arg b(2);
    std::thread t(f, b);
    t.join();
  });
  sw("jthread", [] {
    Fn f;
    Arg b(2);
    std::jthread t(f, b);
  });
  sw("async", [] {
    Fn f;
    Arg b(2);
    auto fut = std::async(std::launch::async, f, b);
    fut.get();
  });
  sw("async deferred", [] {
    Fn f;
    Arg b(2);
    auto fut = std::async(std::launch::deferred, f, b);
    disarm();
    fut.get();
  });
  sw("condition_variable_any and stop_source", [] {
    std::condition_variable_any cv;
    std::stop_source ss;
    std::mutex m;
    std::unique_lock lk(m);
    disarm();
    EXH_EXPECT(!cv.wait_for(lk, std::chrono::milliseconds(0), [] { return false; }), "wait_for");
    EXH_EXPECT(ss.stop_possible() && ss.request_stop(), "stop_source");
    runs = 3;
  });
  // The at-thread-exit registrations, made in a thread started before the failure is armed.
  sweep_new("notify_all_at_thread_exit", [] {
    std::mutex m;
    std::condition_variable cv;
    bool done = false, threw = false;
    long k = st.k;
    st.fired = false;
    std::thread t([&] {
      std::unique_lock lk(m);
      done = true;
      st.left[gnew] = k;
      try {
        std::notify_all_at_thread_exit(cv, std::move(lk));
      } catch (const std::bad_alloc&) {
        threw = true;
        cv.notify_all();  // the by-value lock parameter has released m
      }
      disarm();
    });
    {
      std::unique_lock lk(m);
      cv.wait(lk, [&] { return done; });
    }
    t.join();
    EXH_EXPECT(m.try_lock(), "the mutex is still locked after the thread ended");
    m.unlock();
    return threw || st.fired;
  }, options{true, 4000});  // an implementation may recover from the failure
  sweep_new("set_value_at_thread_exit", [] {
    std::promise<int> p;
    std::future<int> f = p.get_future();
    bool threw = false;
    long k = st.k;
    st.fired = false;
    std::thread t([&] {
      st.left[gnew] = k;
      try {
        p.set_value_at_thread_exit(5);
      } catch (const std::bad_alloc&) {
        threw = true;
      }
      disarm();
    });
    t.join();
    if (threw) {
      EXH_EXPECT(f.wait_for(std::chrono::seconds(0)) == std::future_status::timeout, "made ready although registration failed");
      p.set_value(6);
      EXH_EXPECT(f.get() == 6, "a later set_value");
    } else {
      EXH_EXPECT(f.wait_for(std::chrono::seconds(10)) == std::future_status::ready && f.get() == 5, "set_value_at_thread_exit");
    }
    return threw || st.fired;
  }, options{true, 4000});
  return finish();
}
