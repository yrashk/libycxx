// libycxx hosted: condition variables ([thread.condition]).
//
// __ycxx::__detail::__futex_condvar is a sequence counter on the PAL's address wait: a waiter, holding
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
#include <ycxx/core/stop_token.hpp>
#include <ycxx/hosted/thread_support.hpp>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

class __futex_condvar {
  __ycxx_pal_u32 __seq_ = 0;
  __ycxx_pal_u32 __waiters_ = 0;

  __ycxx_pal_u32 __enter() noexcept {
    __atomic_fetch_add(&__waiters_, 1, __ATOMIC_SEQ_CST);
    return __atomic_load_n(&__seq_, __ATOMIC_SEQ_CST);
  }
  void __y_leave() noexcept { __atomic_fetch_sub(&__waiters_, 1, __ATOMIC_RELEASE); }

public:
  constexpr __futex_condvar() noexcept = default;
  __futex_condvar(const __futex_condvar&) = delete;
  __futex_condvar& operator=(const __futex_condvar&) = delete;
  ~__futex_condvar() {
    while (__atomic_load_n(&__waiters_, __ATOMIC_ACQUIRE) != 0)
      ::__ycxx_pal_thread_yield();
  }

  void notify_one() noexcept {
    __atomic_fetch_add(&__seq_, 1, __ATOMIC_SEQ_CST);
    if (__atomic_load_n(&__waiters_, __ATOMIC_SEQ_CST) != 0)
      ::__ycxx_pal_wake_one(&__seq_);
  }
  void notify_all() noexcept {
    __atomic_fetch_add(&__seq_, 1, __ATOMIC_SEQ_CST);
    if (__atomic_load_n(&__waiters_, __ATOMIC_SEQ_CST) != 0)
      ::__ycxx_pal_wake_all(&__seq_);
  }

  // m is held; it is released while blocked and held again on return.
  template <class _Mp>
  void wait(_Mp& m) noexcept {
    const __ycxx_pal_u32 s = __enter();
    m.unlock();
    ::__ycxx_pal_wait(&__seq_, s);
    __y_leave(); // the last access to *this
    m.lock();
  }
  // As wait, but returns by the deadline: false if it returned because the deadline passed.
  template <class _Mp>
  bool wait_until(_Mp& m, const __pal_deadline& d) noexcept {
    const __ycxx_pal_u32 s = __enter();
    m.unlock();
    const int r = ::__ycxx_pal_wait_until(&__seq_, s, d.clock, d.__sec, d.__nsec);
    __y_leave(); // the last access to *this
    m.lock();
    return r == 0;
  }
};

// The hosted runtime (src/hosted/thread.cpp): runs f(arg) when the calling thread exits, after
// (as far as the platform allows) its thread_local objects are destroyed.
void __at_thread_exit(void (*__f)(void*), void* arg);

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

enum class cv_status { no_timeout, timeout };

// [thread.condition.condvar]
class condition_variable {
  __ycxx::__detail::__futex_condvar __cv_;

public:
  condition_variable() = default;
  ~condition_variable() = default;
  condition_variable(const condition_variable&) = delete;
  condition_variable& operator=(const condition_variable&) = delete;

  void notify_one() noexcept { __cv_.notify_one(); }
  void notify_all() noexcept { __cv_.notify_all(); }

  void wait(unique_lock<mutex>& lock) {
    __ycxx::__detail::__precondition(lock.owns_lock(), "condition_variable::wait: the lock is not held");
    __cv_.wait(*lock.mutex());
  }
  template <class _Predicate>
  void wait(unique_lock<mutex>& lock, _Predicate pred) {
    while (!pred())
      wait(lock);
  }
  template <class _Clock, class _Duration>
  cv_status wait_until(unique_lock<mutex>& lock, const chrono::time_point<_Clock, _Duration>& __abs_time) {
    static_assert(chrono::is_clock_v<_Clock>, "wait_until: Clock must meet the Cpp17Clock requirements");
    __ycxx::__detail::__precondition(lock.owns_lock(), "condition_variable::wait_until: the lock is not held");
    // Clock::now() is read once before the wait and once after it.
    if (const auto now = _Clock::now(); now < __abs_time)
      __cv_.wait_until(*lock.mutex(), __ycxx::__detail::__deadline_at(__abs_time, now));
    return _Clock::now() < __abs_time ? cv_status::no_timeout : cv_status::timeout;
  }
  template <class _Clock, class _Duration, class _Predicate>
  bool wait_until(unique_lock<mutex>& lock, const chrono::time_point<_Clock, _Duration>& __abs_time, _Predicate pred) {
    while (!pred())
      if (wait_until(lock, __abs_time) == cv_status::timeout)
        return pred();
    return true;
  }
  template <class _Rep, class _Period>
  cv_status wait_for(unique_lock<mutex>& lock, const chrono::duration<_Rep, _Period>& __rel_time) {
    return wait_until(lock, __ycxx::__detail::__steady_deadline(__rel_time));
  }
  template <class _Rep, class _Period, class _Predicate>
  bool wait_for(unique_lock<mutex>& lock, const chrono::duration<_Rep, _Period>& __rel_time, _Predicate pred) {
    return wait_until(lock, __ycxx::__detail::__steady_deadline(__rel_time), static_cast<_Predicate&&>(pred));
  }
};

// [thread.condition.nonmember]
inline void notify_all_at_thread_exit(condition_variable& __cond, unique_lock<mutex> __lk) {
  __ycxx::__detail::__precondition(__lk.owns_lock(), "notify_all_at_thread_exit: the lock is not held");
  struct __record {
    condition_variable* __cond;
    mutex* m;
    // [thread.condition.nonmember]/2: "cond.notify_all(); lk.unlock();" -- notified while the
    // lock is held, so a waiter that gets the lock may destroy cond at once.
    static void run(void* p) {
      __record r = *static_cast<__record*>(p);
      delete static_cast<__record*>(p);
      r.__cond->notify_all();
      r.m->unlock();
    }
  };
  struct __owner {
    __record* r;
    ~__owner() { delete r; }
  } __o{new __record{__builtin_addressof(__cond), __lk.mutex()}};
  __ycxx::__detail::__at_thread_exit(&__record::run, __o.r);
  __o.r = nullptr;
  (void)__lk.release();
}

// [thread.condition.condvarany]
class condition_variable_any {
  __ycxx::__detail::__futex_mutex __m_;
  __ycxx::__detail::__futex_condvar __cv_;
  __ycxx_pal_u32 __active_ = 0; // waiters that may still touch m_

  // Re-locks the caller's lock; [thread.condition.condvarany.wait]: terminate if that fails.
  template <class _Lock>
  static void __relock(_Lock& lock) noexcept {
    lock.lock();
  }

  // Holds m_ and counts the caller as active; undone (m_ released) if lock.unlock() throws.
  struct __enter_guard {
    condition_variable_any* __self;
    bool __armed = true;
    ~__enter_guard() {
      if (__armed) {
        __self->__m_.unlock();
        __atomic_fetch_sub(&__self->__active_, 1, __ATOMIC_RELEASE);
      }
    }
  };

  // Blocks once (with the caller's lock released) unless stop_requested() is already true
  // under m_; the stop callback of the interruptible waits notifies under m_ too.
  template <class _Lock, class _StopCheck>
  bool __wait_once(_Lock& lock, const __ycxx::__detail::__pal_deadline* d, _StopCheck __stopped) {
    __m_.lock();
    __atomic_fetch_add(&__active_, 1, __ATOMIC_RELAXED);
    __enter_guard __g{this};
    if (__stopped())
      return true;
    lock.unlock();
    __g.__armed = false;
    bool __woken = true;
    if (d)
      __woken = __cv_.wait_until(__m_, *d);
    else
      __cv_.wait(__m_);
    __m_.unlock();
    __atomic_fetch_sub(&__active_, 1, __ATOMIC_RELEASE);
    __relock(lock);
    return __woken;
  }

public:
  condition_variable_any() = default;
  ~condition_variable_any() {
    while (__atomic_load_n(&__active_, __ATOMIC_ACQUIRE) != 0)
      ::__ycxx_pal_thread_yield();
  }
  condition_variable_any(const condition_variable_any&) = delete;
  condition_variable_any& operator=(const condition_variable_any&) = delete;

  void notify_one() noexcept {
    __m_.lock();
    __m_.unlock();
    __cv_.notify_one();
  }
  void notify_all() noexcept {
    __m_.lock();
    __m_.unlock();
    __cv_.notify_all();
  }

  // [thread.condvarany.wait]
  template <class _Lock>
  void wait(_Lock& lock) {
    __wait_once(lock, nullptr, [] { return false; });
  }
  template <class _Lock, class _Predicate>
  void wait(_Lock& lock, _Predicate pred) {
    while (!pred())
      wait(lock);
  }
  template <class _Lock, class _Clock, class _Duration>
  cv_status wait_until(_Lock& lock, const chrono::time_point<_Clock, _Duration>& __abs_time) {
    static_assert(chrono::is_clock_v<_Clock>, "wait_until: Clock must meet the Cpp17Clock requirements");
    if (const auto now = _Clock::now(); now < __abs_time) {
      const __ycxx::__detail::__pal_deadline d = __ycxx::__detail::__deadline_at(__abs_time, now);
      __wait_once(lock, &d, [] { return false; });
    }
    return _Clock::now() < __abs_time ? cv_status::no_timeout : cv_status::timeout;
  }
  template <class _Lock, class _Clock, class _Duration, class _Predicate>
  bool wait_until(_Lock& lock, const chrono::time_point<_Clock, _Duration>& __abs_time, _Predicate pred) {
    while (!pred())
      if (wait_until(lock, __abs_time) == cv_status::timeout)
        return pred();
    return true;
  }
  template <class _Lock, class _Rep, class _Period>
  cv_status wait_for(_Lock& lock, const chrono::duration<_Rep, _Period>& __rel_time) {
    return wait_until(lock, __ycxx::__detail::__steady_deadline(__rel_time));
  }
  template <class _Lock, class _Rep, class _Period, class _Predicate>
  bool wait_for(_Lock& lock, const chrono::duration<_Rep, _Period>& __rel_time, _Predicate pred) {
    return wait_until(lock, __ycxx::__detail::__steady_deadline(__rel_time), static_cast<_Predicate&&>(pred));
  }

  // [thread.condvarany.intwait]
  template <class _Lock, class _Predicate>
  bool wait(_Lock& lock, stop_token __stoken, _Predicate pred) {
    if (__stoken.stop_requested())
      return pred();
    auto __notify = [this]() noexcept { notify_all(); };
    stop_callback<decltype(__notify)> __cb(__stoken, __notify);
    while (!__stoken.stop_requested()) {
      if (pred())
        return true;
      __wait_once(lock, nullptr, [&] { return __stoken.stop_requested(); });
    }
    return pred();
  }
  template <class _Lock, class _Clock, class _Duration, class _Predicate>
  bool wait_until(_Lock& lock, stop_token __stoken, const chrono::time_point<_Clock, _Duration>& __abs_time, _Predicate pred) {
    static_assert(chrono::is_clock_v<_Clock>, "wait_until: Clock must meet the Cpp17Clock requirements");
    if (__stoken.stop_requested())
      return pred();
    auto __notify = [this]() noexcept { notify_all(); };
    stop_callback<decltype(__notify)> __cb(__stoken, __notify);
    while (!__stoken.stop_requested()) {
      if (pred())
        return true;
      const auto now = _Clock::now();
      if (!(now < __abs_time))
        return pred();
      const __ycxx::__detail::__pal_deadline d = __ycxx::__detail::__deadline_at(__abs_time, now);
      __wait_once(lock, &d, [&] { return __stoken.stop_requested(); });
    }
    return pred();
  }
  template <class _Lock, class _Rep, class _Period, class _Predicate>
  bool wait_for(_Lock& lock, stop_token __stoken, const chrono::duration<_Rep, _Period>& __rel_time, _Predicate pred) {
    return wait_until(lock, static_cast<stop_token&&>(__stoken), __ycxx::__detail::__steady_deadline(__rel_time),
                      static_cast<_Predicate&&>(pred));
  }
};

} // namespace std
