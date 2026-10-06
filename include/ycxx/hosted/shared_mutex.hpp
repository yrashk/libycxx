// libycxx hosted: shared mutexes and shared_lock ([thread.sharedmutex], [thread.lock.shared]).
//
// The two-gate algorithm: one internal futex mutex guards a state word (a "writer entered" bit
// and the reader count). A writer passes gate 1 by setting the bit (no new readers enter after
// that) and then waits at gate 2 for the readers to drain. Writers therefore do not starve.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/hosted/chrono_clocks.hpp>
#include <ycxx/hosted/condition_variable.hpp>
#include <ycxx/hosted/mutex.hpp>
#include <ycxx/hosted/thread_support.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

class __shared_futex_mutex {
  static constexpr unsigned __write_entered = 1u << 31;
  static constexpr unsigned __max_readers = ~__write_entered;

  __futex_mutex __m_;
  __futex_condvar __gate1_;
  __futex_condvar __gate2_;
  unsigned __state_ = 0;

  struct __hold {
    __futex_mutex& m;
    explicit __hold(__futex_mutex& __x) noexcept : m(__x) { m.lock(); }
    ~__hold() { m.unlock(); }
  };

  // Waits on cv until done() or the deadline (any clock) passes; false on timeout.
  template <class _Done, class _Clock, class _Duration>
  bool __wait_for_state(__futex_condvar& __cv, _Done done, const std::chrono::time_point<_Clock, _Duration>* abs) {
    while (!done()) {
      if (!abs) {
        __cv.wait(__m_);
        continue;
      }
      const auto now = _Clock::now();
      if (!(now < *abs))
        return done();
      __cv.wait_until(__m_, ::__ycxx::__detail::__deadline_at(*abs, now));
    }
    return true;
  }

public:
  constexpr __shared_futex_mutex() noexcept = default;

  template <class _Clock = std::chrono::steady_clock, class _Duration = typename _Clock::duration>
  bool __lock_impl(const std::chrono::time_point<_Clock, _Duration>* abs = nullptr) {
    __hold h(__m_);
    if (!__wait_for_state(__gate1_, [this] { return (__state_ & __write_entered) == 0; }, abs))
      return false;
    __state_ |= __write_entered;
    // Gate 2 gives the write bit back when the deadline passes or Clock::now() throws.
    struct __give_back {
      __shared_futex_mutex* __self;
      ~__give_back() {
        if (__self) {
          __self->__state_ &= ~__write_entered;
          __self->__gate1_.notify_all();
        }
      }
    } __g{this};
    if (!__wait_for_state(__gate2_, [this] { return (__state_ & __max_readers) == 0; }, abs))
      return false;
    __g.__self = nullptr;
    return true;
  }
  bool try_lock() noexcept {
    __hold h(__m_);
    if (__state_ != 0)
      return false;
    __state_ = __write_entered;
    return true;
  }
  void unlock() noexcept {
    {
      __hold h(__m_);
      __state_ = 0;
    }
    __gate1_.notify_all();
  }

  template <class _Clock = std::chrono::steady_clock, class _Duration = typename _Clock::duration>
  bool __lock_shared_impl(const std::chrono::time_point<_Clock, _Duration>* abs = nullptr) {
    __hold h(__m_);
    if (!__wait_for_state(
            __gate1_, [this] { return (__state_ & __write_entered) == 0 && (__state_ & __max_readers) != __max_readers; }, abs))
      return false;
    ++__state_;
    return true;
  }
  bool try_lock_shared() noexcept {
    __hold h(__m_);
    if ((__state_ & __write_entered) != 0 || (__state_ & __max_readers) == __max_readers)
      return false;
    ++__state_;
    return true;
  }
  void unlock_shared() noexcept {
    __hold h(__m_);
    const unsigned __readers = (__state_ & __max_readers) - 1;
    __state_ = (__state_ & __write_entered) | __readers;
    if (__state_ & __write_entered) {
      if (__readers == 0)
        __gate2_.notify_one();
    } else if (__readers == __max_readers - 1) {
      __gate1_.notify_one();
    }
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [thread.sharedmutex.class]
class shared_mutex {
  __ycxx::__detail::__shared_futex_mutex __m_;

public:
  shared_mutex() = default;
  ~shared_mutex() = default;
  shared_mutex(const shared_mutex&) = delete;
  shared_mutex& operator=(const shared_mutex&) = delete;

  void lock() { __m_.__lock_impl(); }
  [[nodiscard]] bool try_lock() { return __m_.try_lock(); }
  void unlock() { __m_.unlock(); }
  void lock_shared() { __m_.__lock_shared_impl(); }
  [[nodiscard]] bool try_lock_shared() { return __m_.try_lock_shared(); }
  void unlock_shared() { __m_.unlock_shared(); }
};

// [thread.sharedtimedmutex.class]
class shared_timed_mutex {
  __ycxx::__detail::__shared_futex_mutex __m_;

public:
  shared_timed_mutex() = default;
  ~shared_timed_mutex() = default;
  shared_timed_mutex(const shared_timed_mutex&) = delete;
  shared_timed_mutex& operator=(const shared_timed_mutex&) = delete;

  void lock() { __m_.__lock_impl(); }
  [[nodiscard]] bool try_lock() { return __m_.try_lock(); }
  template <class _Rep, class _Period>
  [[nodiscard]] bool try_lock_for(const chrono::duration<_Rep, _Period>& __rel_time) {
    return try_lock_until(__ycxx::__detail::__steady_deadline(__rel_time));
  }
  template <class _Clock, class _Duration>
  [[nodiscard]] bool try_lock_until(const chrono::time_point<_Clock, _Duration>& __abs_time) {
    static_assert(chrono::is_clock_v<_Clock>, "try_lock_until: Clock must meet the Cpp17Clock requirements");
    return __m_.__lock_impl(__builtin_addressof(__abs_time));
  }
  void unlock() { __m_.unlock(); }

  void lock_shared() { __m_.__lock_shared_impl(); }
  [[nodiscard]] bool try_lock_shared() { return __m_.try_lock_shared(); }
  template <class _Rep, class _Period>
  [[nodiscard]] bool try_lock_shared_for(const chrono::duration<_Rep, _Period>& __rel_time) {
    return try_lock_shared_until(__ycxx::__detail::__steady_deadline(__rel_time));
  }
  template <class _Clock, class _Duration>
  [[nodiscard]] bool try_lock_shared_until(const chrono::time_point<_Clock, _Duration>& __abs_time) {
    static_assert(chrono::is_clock_v<_Clock>, "try_lock_shared_until: Clock must meet the Cpp17Clock requirements");
    return __m_.__lock_shared_impl(__builtin_addressof(__abs_time));
  }
  void unlock_shared() { __m_.unlock_shared(); }
};

// [thread.lock.shared]
template <class _Mutex>
class shared_lock {
  _Mutex* __pm_ = nullptr;
  bool __owns_ = false;

  void __check_lockable(const char* what) const {
    if (!__pm_)
      __ycxx::__detail::__raise_system_error(errc::operation_not_permitted, what);
    if (__owns_)
      __ycxx::__detail::__raise_system_error(errc::resource_deadlock_would_occur, what);
  }

public:
  using mutex_type = _Mutex;

  shared_lock() noexcept = default;
  explicit shared_lock(mutex_type& m) : __pm_(__builtin_addressof(m)) {
    m.lock_shared();
    __owns_ = true;
  }
  shared_lock(mutex_type& m, defer_lock_t) noexcept : __pm_(__builtin_addressof(m)) {}
  shared_lock(mutex_type& m, try_to_lock_t) : __pm_(__builtin_addressof(m)), __owns_(m.try_lock_shared()) {}
  shared_lock(mutex_type& m, adopt_lock_t) : __pm_(__builtin_addressof(m)), __owns_(true) {}
  template <class _Clock, class _Duration>
  shared_lock(mutex_type& m, const chrono::time_point<_Clock, _Duration>& __abs_time)
      : __pm_(__builtin_addressof(m)), __owns_(m.try_lock_shared_until(__abs_time)) {}
  template <class _Rep, class _Period>
  shared_lock(mutex_type& m, const chrono::duration<_Rep, _Period>& __rel_time)
      : __pm_(__builtin_addressof(m)), __owns_(m.try_lock_shared_for(__rel_time)) {}
  ~shared_lock() {
    if (__owns_)
      __pm_->unlock_shared();
  }
  shared_lock(const shared_lock&) = delete;
  shared_lock& operator=(const shared_lock&) = delete;
  shared_lock(shared_lock&& __sl) noexcept : __pm_(__sl.__pm_), __owns_(__sl.__owns_) {
    __sl.__pm_ = nullptr;
    __sl.__owns_ = false;
  }
  shared_lock& operator=(shared_lock&& __sl) noexcept {
    shared_lock(static_cast<shared_lock&&>(__sl)).swap(*this);
    return *this;
  }

  void lock() {
    __check_lockable("shared_lock::lock");
    __pm_->lock_shared();
    __owns_ = true;
  }
  [[nodiscard]] bool try_lock() {
    __check_lockable("shared_lock::try_lock");
    __owns_ = __pm_->try_lock_shared();
    return __owns_;
  }
  template <class _Rep, class _Period>
  [[nodiscard]] bool try_lock_for(const chrono::duration<_Rep, _Period>& __rel_time) {
    __check_lockable("shared_lock::try_lock_for");
    __owns_ = __pm_->try_lock_shared_for(__rel_time);
    return __owns_;
  }
  template <class _Clock, class _Duration>
  [[nodiscard]] bool try_lock_until(const chrono::time_point<_Clock, _Duration>& __abs_time) {
    __check_lockable("shared_lock::try_lock_until");
    __owns_ = __pm_->try_lock_shared_until(__abs_time);
    return __owns_;
  }
  void unlock() {
    if (!__owns_)
      __ycxx::__detail::__raise_system_error(errc::operation_not_permitted, "shared_lock::unlock: the lock is not owned");
    __pm_->unlock_shared();
    __owns_ = false;
  }

  void swap(shared_lock& __u) noexcept {
    _Mutex* m = __pm_;
    __pm_ = __u.__pm_;
    __u.__pm_ = m;
    bool __o = __owns_;
    __owns_ = __u.__owns_;
    __u.__owns_ = __o;
  }
  mutex_type* release() noexcept {
    _Mutex* m = __pm_;
    __pm_ = nullptr;
    __owns_ = false;
    return m;
  }

  [[nodiscard]] bool owns_lock() const noexcept { return __owns_; }
  explicit operator bool() const noexcept { return __owns_; }
  [[nodiscard]] mutex_type* mutex() const noexcept { return __pm_; }
};

template <class _Mutex>
void swap(shared_lock<_Mutex>& __x, shared_lock<_Mutex>& y) noexcept {
  __x.swap(y);
}

} // namespace std
