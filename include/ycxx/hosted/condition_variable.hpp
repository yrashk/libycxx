// libycxx hosted: condition variables ([thread.condition]).
//
// ycxx::detail::futex_condvar is a sequence counter on the PAL's address wait: a waiter, holding
// the mutex, registers (waiters_) and reads the sequence, unlocks, and blocks while the sequence
// is unchanged; a notification bumps the sequence and wakes when there are registered waiters
// (both sides use seq_cst operations, so either the notifier sees the waiter or the waiter's futex
// wait sees the new sequence). A waiter deregisters as soon as its futex wait returns, before it
// locks the mutex again, and the destructor waits for the count to drop to zero:
// [thread.condition.condvar] allows destroying a condition variable once every waiter is
// notified, while they are still returning from wait (possibly while the destroying thread
// holds the mutex they need).
//
// condition_variable_any adds its own futex mutex around the user's lock, and a second count so
// that its destructor also waits for waiters to finish with that mutex.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/hosted/chrono_clocks.hpp>
#include <ycxx/hosted/mutex.hpp>
#include <ycxx/hosted/stop_token.hpp>
#include <ycxx/hosted/thread_support.hpp>
#include <ycxx/pal.h>

namespace ycxx::detail {

class futex_condvar {
  ycxx_pal_u32 seq_ = 0;
  ycxx_pal_u32 waiters_ = 0;

  ycxx_pal_u32 enter() noexcept {
    __atomic_fetch_add(&waiters_, 1, __ATOMIC_SEQ_CST);
    return __atomic_load_n(&seq_, __ATOMIC_SEQ_CST);
  }
  void leave() noexcept { __atomic_fetch_sub(&waiters_, 1, __ATOMIC_RELEASE); }

public:
  constexpr futex_condvar() noexcept = default;
  futex_condvar(const futex_condvar&) = delete;
  futex_condvar& operator=(const futex_condvar&) = delete;
  ~futex_condvar() {
    while (__atomic_load_n(&waiters_, __ATOMIC_ACQUIRE) != 0)
      ::ycxx_pal_thread_yield();
  }

  void notify_one() noexcept {
    __atomic_fetch_add(&seq_, 1, __ATOMIC_SEQ_CST);
    if (__atomic_load_n(&waiters_, __ATOMIC_SEQ_CST) != 0)
      ::ycxx_pal_wake_one(&seq_);
  }
  void notify_all() noexcept {
    __atomic_fetch_add(&seq_, 1, __ATOMIC_SEQ_CST);
    if (__atomic_load_n(&waiters_, __ATOMIC_SEQ_CST) != 0)
      ::ycxx_pal_wake_all(&seq_);
  }

  // m is held; it is released while blocked and held again on return.
  template <class M>
  void wait(M& m) noexcept {
    const ycxx_pal_u32 s = enter();
    m.unlock();
    ::ycxx_pal_wait(&seq_, s);
    leave(); // the last access to *this
    m.lock();
  }
  // As wait, but returns by the deadline: false if it returned because the deadline passed.
  template <class M>
  bool wait_until(M& m, const pal_deadline& d) noexcept {
    const ycxx_pal_u32 s = enter();
    m.unlock();
    const int r = ::ycxx_pal_wait_until(&seq_, s, d.clock, d.sec, d.nsec);
    leave(); // the last access to *this
    m.lock();
    return r == 0;
  }
};

// The hosted runtime (src/hosted/thread.cpp): runs f(arg) when the calling thread exits, after
// (as far as the platform allows) its thread_local objects are destroyed.
void at_thread_exit(void (*f)(void*), void* arg);

} // namespace ycxx::detail

namespace std {

enum class cv_status { no_timeout, timeout };

// [thread.condition.condvar]
class condition_variable {
  ycxx::detail::futex_condvar cv_;

public:
  condition_variable() = default;
  ~condition_variable() = default;
  condition_variable(const condition_variable&) = delete;
  condition_variable& operator=(const condition_variable&) = delete;

  void notify_one() noexcept { cv_.notify_one(); }
  void notify_all() noexcept { cv_.notify_all(); }

  void wait(unique_lock<mutex>& lock) {
    ycxx::detail::precondition(lock.owns_lock(), "condition_variable::wait: the lock is not held");
    cv_.wait(*lock.mutex());
  }
  template <class Predicate>
  void wait(unique_lock<mutex>& lock, Predicate pred) {
    while (!pred())
      wait(lock);
  }
  template <class Clock, class Duration>
  cv_status wait_until(unique_lock<mutex>& lock, const chrono::time_point<Clock, Duration>& abs_time) {
    static_assert(chrono::is_clock_v<Clock>, "wait_until: Clock must meet the Cpp17Clock requirements");
    ycxx::detail::precondition(lock.owns_lock(), "condition_variable::wait_until: the lock is not held");
    if (Clock::now() < abs_time)
      cv_.wait_until(*lock.mutex(), ycxx::detail::deadline_at(abs_time));
    return Clock::now() < abs_time ? cv_status::no_timeout : cv_status::timeout;
  }
  template <class Clock, class Duration, class Predicate>
  bool wait_until(unique_lock<mutex>& lock, const chrono::time_point<Clock, Duration>& abs_time, Predicate pred) {
    while (!pred())
      if (wait_until(lock, abs_time) == cv_status::timeout)
        return pred();
    return true;
  }
  template <class Rep, class Period>
  cv_status wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time) {
    return wait_until(lock, ycxx::detail::steady_deadline(rel_time));
  }
  template <class Rep, class Period, class Predicate>
  bool wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time, Predicate pred) {
    return wait_until(lock, ycxx::detail::steady_deadline(rel_time), static_cast<Predicate&&>(pred));
  }
};

// [thread.condition.nonmember]
inline void notify_all_at_thread_exit(condition_variable& cond, unique_lock<mutex> lk) {
  ycxx::detail::precondition(lk.owns_lock(), "notify_all_at_thread_exit: the lock is not held");
  struct record {
    condition_variable* cond;
    mutex* m;
    // [thread.condition.nonmember]/2: "cond.notify_all(); lk.unlock();" -- notified while the
    // lock is held, so a waiter that gets the lock may destroy cond at once.
    static void run(void* p) {
      record r = *static_cast<record*>(p);
      delete static_cast<record*>(p);
      r.cond->notify_all();
      r.m->unlock();
    }
  };
  struct owner {
    record* r;
    ~owner() { delete r; }
  } o{new record{__builtin_addressof(cond), lk.mutex()}};
  ycxx::detail::at_thread_exit(&record::run, o.r);
  o.r = nullptr;
  (void)lk.release();
}

// [thread.condition.condvarany]
class condition_variable_any {
  ycxx::detail::futex_mutex m_;
  ycxx::detail::futex_condvar cv_;
  ycxx_pal_u32 active_ = 0; // waiters that may still touch m_

  // Re-locks the caller's lock; [thread.condition.condvarany.wait]: terminate if that fails.
  template <class Lock>
  static void relock(Lock& lock) noexcept {
    lock.lock();
  }

  // Holds m_ and counts the caller as active; undone (m_ released) if lock.unlock() throws.
  struct enter_guard {
    condition_variable_any* self;
    bool armed = true;
    ~enter_guard() {
      if (armed) {
        self->m_.unlock();
        __atomic_fetch_sub(&self->active_, 1, __ATOMIC_RELEASE);
      }
    }
  };

  // Blocks once (with the caller's lock released) unless stop_requested() is already true
  // under m_; the stop callback of the interruptible waits notifies under m_ too.
  template <class Lock, class StopCheck>
  bool wait_once(Lock& lock, const ycxx::detail::pal_deadline* d, StopCheck stopped) {
    m_.lock();
    __atomic_fetch_add(&active_, 1, __ATOMIC_RELAXED);
    enter_guard g{this};
    if (stopped())
      return true;
    lock.unlock();
    g.armed = false;
    bool woken = true;
    if (d)
      woken = cv_.wait_until(m_, *d);
    else
      cv_.wait(m_);
    m_.unlock();
    __atomic_fetch_sub(&active_, 1, __ATOMIC_RELEASE);
    relock(lock);
    return woken;
  }

public:
  condition_variable_any() = default;
  ~condition_variable_any() {
    while (__atomic_load_n(&active_, __ATOMIC_ACQUIRE) != 0)
      ::ycxx_pal_thread_yield();
  }
  condition_variable_any(const condition_variable_any&) = delete;
  condition_variable_any& operator=(const condition_variable_any&) = delete;

  void notify_one() noexcept {
    m_.lock();
    m_.unlock();
    cv_.notify_one();
  }
  void notify_all() noexcept {
    m_.lock();
    m_.unlock();
    cv_.notify_all();
  }

  // [thread.condvarany.wait]
  template <class Lock>
  void wait(Lock& lock) {
    wait_once(lock, nullptr, [] { return false; });
  }
  template <class Lock, class Predicate>
  void wait(Lock& lock, Predicate pred) {
    while (!pred())
      wait(lock);
  }
  template <class Lock, class Clock, class Duration>
  cv_status wait_until(Lock& lock, const chrono::time_point<Clock, Duration>& abs_time) {
    static_assert(chrono::is_clock_v<Clock>, "wait_until: Clock must meet the Cpp17Clock requirements");
    if (Clock::now() < abs_time) {
      const ycxx::detail::pal_deadline d = ycxx::detail::deadline_at(abs_time);
      wait_once(lock, &d, [] { return false; });
    }
    return Clock::now() < abs_time ? cv_status::no_timeout : cv_status::timeout;
  }
  template <class Lock, class Clock, class Duration, class Predicate>
  bool wait_until(Lock& lock, const chrono::time_point<Clock, Duration>& abs_time, Predicate pred) {
    while (!pred())
      if (wait_until(lock, abs_time) == cv_status::timeout)
        return pred();
    return true;
  }
  template <class Lock, class Rep, class Period>
  cv_status wait_for(Lock& lock, const chrono::duration<Rep, Period>& rel_time) {
    return wait_until(lock, ycxx::detail::steady_deadline(rel_time));
  }
  template <class Lock, class Rep, class Period, class Predicate>
  bool wait_for(Lock& lock, const chrono::duration<Rep, Period>& rel_time, Predicate pred) {
    return wait_until(lock, ycxx::detail::steady_deadline(rel_time), static_cast<Predicate&&>(pred));
  }

  // [thread.condvarany.intwait]
  template <class Lock, class Predicate>
  bool wait(Lock& lock, stop_token stoken, Predicate pred) {
    if (stoken.stop_requested())
      return pred();
    auto notify = [this]() noexcept { notify_all(); };
    stop_callback<decltype(notify)> cb(stoken, notify);
    while (!stoken.stop_requested()) {
      if (pred())
        return true;
      wait_once(lock, nullptr, [&] { return stoken.stop_requested(); });
    }
    return pred();
  }
  template <class Lock, class Clock, class Duration, class Predicate>
  bool wait_until(Lock& lock, stop_token stoken, const chrono::time_point<Clock, Duration>& abs_time, Predicate pred) {
    static_assert(chrono::is_clock_v<Clock>, "wait_until: Clock must meet the Cpp17Clock requirements");
    if (stoken.stop_requested())
      return pred();
    auto notify = [this]() noexcept { notify_all(); };
    stop_callback<decltype(notify)> cb(stoken, notify);
    while (!stoken.stop_requested()) {
      if (pred())
        return true;
      if (!(Clock::now() < abs_time))
        return pred();
      const ycxx::detail::pal_deadline d = ycxx::detail::deadline_at(abs_time);
      wait_once(lock, &d, [&] { return stoken.stop_requested(); });
    }
    return pred();
  }
  template <class Lock, class Rep, class Period, class Predicate>
  bool wait_for(Lock& lock, stop_token stoken, const chrono::duration<Rep, Period>& rel_time, Predicate pred) {
    return wait_until(lock, static_cast<stop_token&&>(stoken), ycxx::detail::steady_deadline(rel_time),
                      static_cast<Predicate&&>(pred));
  }
};

} // namespace std
