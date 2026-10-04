// [thread.timedmutex.requirements.general]/11: try_lock_until(abs_time) "returns before the
// absolute timeout ([thread.req.timing]) specified by abs_time only if it has obtained
// ownership of the mutex object"; /14: returns true iff ownership was obtained; /15: "Throws:
// Timeout-related exceptions". [thread.req.timing]/4: for an absolute timeout "Implementations
// should use the clock specified in the time point"; the return happens no earlier than Ct for
// a clock that is not adjusted. /8: "A function that takes an argument which specifies a
// timeout will throw if, during its execution, a clock, time point, or time duration throws an
// exception." Checked with a user-defined Cpp17Clock (millisecond ticks, its own epoch) for
// timed_mutex, recursive_timed_mutex and shared_timed_mutex (exclusive and shared,
// [thread.sharedtimedmutex.requirements.general]), and unique_lock / shared_lock's timed
// constructors ([thread.lock.unique.cons]/17, [thread.lock.shared.cons]/13), while another
// thread holds the mutex. The holder releases only after the timed call has returned or
// thrown, so the call cannot obtain ownership.
// FLAGS: -pthread
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include "check.hpp"
#include "test_clocks.hpp"
#include "watchdog.hpp"

using namespace std::chrono_literals;

// runs body() while another thread holds m exclusively
template<class M, class F>
void while_held(M& m, F body) {
  std::atomic<int> st(0);
  std::thread holder([&] {
    m.lock();
    st.store(1);
    st.notify_one();
    st.wait(1);  // until body() is done
    m.unlock();
  });
  st.wait(0);
  body();
  st.store(2);
  st.notify_one();
  holder.join();
}

template<class M>
void run() {
  M m;
  while_held(m, [&] {
    auto dl = offset_clock::now() + 15ms;
    CHECK(!m.try_lock_until(dl));
    CHECK(offset_clock::now() >= dl);
    // an absolute time that has passed: as if by try_lock(); fails, the mutex is held
    CHECK(!m.try_lock_until(offset_clock::now() - 1h));
    auto dl2 = offset_clock::now() + 5ms;
    std::unique_lock<M> l(m, dl2);
    CHECK(!l.owns_lock() && l.mutex() == &m);
    CHECK(offset_clock::now() >= dl2);
    auto dl3 = offset_clock::now() + 5ms;
    CHECK(!l.try_lock_until(dl3));
    CHECK(offset_clock::now() >= dl3);

    // timeout-related exceptions propagate
    auto tp = throwing_clock::now() + 10ms;
    throwing_clock::armed = true;
    bool thrown = false;
    try {
      (void)m.try_lock_until(tp);
    } catch (clock_error) {
      thrown = true;
    }
    throwing_clock::armed = false;
    CHECK(thrown);
  });
  // free: the user clock works for a successful call too
  CHECK(m.try_lock_until(offset_clock::now() + 1s));
  m.unlock();
}

int main() {
  watchdog(20);
  run<std::timed_mutex>();
  run<std::recursive_timed_mutex>();
  run<std::shared_timed_mutex>();

  std::shared_timed_mutex sm;
  while_held(sm, [&] {
    auto dl = offset_clock::now() + 10ms;
    CHECK(!sm.try_lock_shared_until(dl));
    CHECK(offset_clock::now() >= dl);
    auto dl2 = offset_clock::now() + 5ms;
    std::shared_lock<std::shared_timed_mutex> sl(sm, dl2);
    CHECK(!sl.owns_lock());
    CHECK(offset_clock::now() >= dl2);
    auto tp = throwing_clock::now() + 10ms;
    throwing_clock::armed = true;
    bool thrown = false;
    try {
      (void)sm.try_lock_shared_until(tp);
    } catch (clock_error) {
      thrown = true;
    }
    throwing_clock::armed = false;
    CHECK(thrown);
  });
  CHECK(sm.try_lock_shared_until(offset_clock::now() + 1s));
  sm.unlock_shared();
  return 0;
}
