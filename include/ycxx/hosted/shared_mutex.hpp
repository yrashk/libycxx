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

namespace ycxx::detail {

class shared_futex_mutex {
  static constexpr unsigned write_entered = 1u << 31;
  static constexpr unsigned max_readers = ~write_entered;

  futex_mutex m_;
  futex_condvar gate1_;
  futex_condvar gate2_;
  unsigned state_ = 0;

  struct hold {
    futex_mutex& m;
    explicit hold(futex_mutex& x) noexcept : m(x) { m.lock(); }
    ~hold() { m.unlock(); }
  };

  // Waits on cv until done() or the deadline (any clock) passes; false on timeout.
  template <class Done, class Clock, class Duration>
  bool wait_for_state(futex_condvar& cv, Done done, const std::chrono::time_point<Clock, Duration>* abs) {
    while (!done()) {
      if (!abs) {
        cv.wait(m_);
        continue;
      }
      if (!(Clock::now() < *abs))
        return done();
      cv.wait_until(m_, ::ycxx::detail::deadline_at(*abs));
    }
    return true;
  }

public:
  constexpr shared_futex_mutex() noexcept = default;

  template <class Clock = std::chrono::steady_clock, class Duration = typename Clock::duration>
  bool lock_impl(const std::chrono::time_point<Clock, Duration>* abs = nullptr) {
    hold h(m_);
    if (!wait_for_state(gate1_, [this] { return (state_ & write_entered) == 0; }, abs))
      return false;
    state_ |= write_entered;
    if (!wait_for_state(gate2_, [this] { return (state_ & max_readers) == 0; }, abs)) {
      state_ &= ~write_entered;
      gate1_.notify_all();
      return false;
    }
    return true;
  }
  bool try_lock() noexcept {
    hold h(m_);
    if (state_ != 0)
      return false;
    state_ = write_entered;
    return true;
  }
  void unlock() noexcept {
    {
      hold h(m_);
      state_ = 0;
    }
    gate1_.notify_all();
  }

  template <class Clock = std::chrono::steady_clock, class Duration = typename Clock::duration>
  bool lock_shared_impl(const std::chrono::time_point<Clock, Duration>* abs = nullptr) {
    hold h(m_);
    if (!wait_for_state(
            gate1_, [this] { return (state_ & write_entered) == 0 && (state_ & max_readers) != max_readers; }, abs))
      return false;
    ++state_;
    return true;
  }
  bool try_lock_shared() noexcept {
    hold h(m_);
    if ((state_ & write_entered) != 0 || (state_ & max_readers) == max_readers)
      return false;
    ++state_;
    return true;
  }
  void unlock_shared() noexcept {
    hold h(m_);
    const unsigned readers = (state_ & max_readers) - 1;
    state_ = (state_ & write_entered) | readers;
    if (state_ & write_entered) {
      if (readers == 0)
        gate2_.notify_one();
    } else if (readers == max_readers - 1) {
      gate1_.notify_one();
    }
  }
};

} // namespace ycxx::detail

namespace std {

// [thread.sharedmutex.class]
class shared_mutex {
  ycxx::detail::shared_futex_mutex m_;

public:
  shared_mutex() = default;
  ~shared_mutex() = default;
  shared_mutex(const shared_mutex&) = delete;
  shared_mutex& operator=(const shared_mutex&) = delete;

  void lock() { m_.lock_impl(); }
  [[nodiscard]] bool try_lock() { return m_.try_lock(); }
  void unlock() { m_.unlock(); }
  void lock_shared() { m_.lock_shared_impl(); }
  [[nodiscard]] bool try_lock_shared() { return m_.try_lock_shared(); }
  void unlock_shared() { m_.unlock_shared(); }
};

// [thread.sharedtimedmutex.class]
class shared_timed_mutex {
  ycxx::detail::shared_futex_mutex m_;

public:
  shared_timed_mutex() = default;
  ~shared_timed_mutex() = default;
  shared_timed_mutex(const shared_timed_mutex&) = delete;
  shared_timed_mutex& operator=(const shared_timed_mutex&) = delete;

  void lock() { m_.lock_impl(); }
  [[nodiscard]] bool try_lock() { return m_.try_lock(); }
  template <class Rep, class Period>
  [[nodiscard]] bool try_lock_for(const chrono::duration<Rep, Period>& rel_time) {
    return try_lock_until(ycxx::detail::steady_deadline(rel_time));
  }
  template <class Clock, class Duration>
  [[nodiscard]] bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time) {
    static_assert(chrono::is_clock_v<Clock>, "try_lock_until: Clock must meet the Cpp17Clock requirements");
    return m_.lock_impl(__builtin_addressof(abs_time));
  }
  void unlock() { m_.unlock(); }

  void lock_shared() { m_.lock_shared_impl(); }
  [[nodiscard]] bool try_lock_shared() { return m_.try_lock_shared(); }
  template <class Rep, class Period>
  [[nodiscard]] bool try_lock_shared_for(const chrono::duration<Rep, Period>& rel_time) {
    return try_lock_shared_until(ycxx::detail::steady_deadline(rel_time));
  }
  template <class Clock, class Duration>
  [[nodiscard]] bool try_lock_shared_until(const chrono::time_point<Clock, Duration>& abs_time) {
    static_assert(chrono::is_clock_v<Clock>, "try_lock_shared_until: Clock must meet the Cpp17Clock requirements");
    return m_.lock_shared_impl(__builtin_addressof(abs_time));
  }
  void unlock_shared() { m_.unlock_shared(); }
};

// [thread.lock.shared]
template <class Mutex>
class shared_lock {
  Mutex* pm_ = nullptr;
  bool owns_ = false;

  void check_lockable(const char* what) const {
    if (!pm_)
      ycxx::detail::raise_system_error(errc::operation_not_permitted, what);
    if (owns_)
      ycxx::detail::raise_system_error(errc::resource_deadlock_would_occur, what);
  }

public:
  using mutex_type = Mutex;

  shared_lock() noexcept = default;
  explicit shared_lock(mutex_type& m) : pm_(__builtin_addressof(m)) {
    m.lock_shared();
    owns_ = true;
  }
  shared_lock(mutex_type& m, defer_lock_t) noexcept : pm_(__builtin_addressof(m)) {}
  shared_lock(mutex_type& m, try_to_lock_t) : pm_(__builtin_addressof(m)), owns_(m.try_lock_shared()) {}
  shared_lock(mutex_type& m, adopt_lock_t) : pm_(__builtin_addressof(m)), owns_(true) {}
  template <class Clock, class Duration>
  shared_lock(mutex_type& m, const chrono::time_point<Clock, Duration>& abs_time)
      : pm_(__builtin_addressof(m)), owns_(m.try_lock_shared_until(abs_time)) {}
  template <class Rep, class Period>
  shared_lock(mutex_type& m, const chrono::duration<Rep, Period>& rel_time)
      : pm_(__builtin_addressof(m)), owns_(m.try_lock_shared_for(rel_time)) {}
  ~shared_lock() {
    if (owns_)
      pm_->unlock_shared();
  }
  shared_lock(const shared_lock&) = delete;
  shared_lock& operator=(const shared_lock&) = delete;
  shared_lock(shared_lock&& sl) noexcept : pm_(sl.pm_), owns_(sl.owns_) {
    sl.pm_ = nullptr;
    sl.owns_ = false;
  }
  shared_lock& operator=(shared_lock&& sl) noexcept {
    shared_lock(static_cast<shared_lock&&>(sl)).swap(*this);
    return *this;
  }

  void lock() {
    check_lockable("shared_lock::lock");
    pm_->lock_shared();
    owns_ = true;
  }
  [[nodiscard]] bool try_lock() {
    check_lockable("shared_lock::try_lock");
    owns_ = pm_->try_lock_shared();
    return owns_;
  }
  template <class Rep, class Period>
  [[nodiscard]] bool try_lock_for(const chrono::duration<Rep, Period>& rel_time) {
    check_lockable("shared_lock::try_lock_for");
    owns_ = pm_->try_lock_shared_for(rel_time);
    return owns_;
  }
  template <class Clock, class Duration>
  [[nodiscard]] bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time) {
    check_lockable("shared_lock::try_lock_until");
    owns_ = pm_->try_lock_shared_until(abs_time);
    return owns_;
  }
  void unlock() {
    if (!owns_)
      ycxx::detail::raise_system_error(errc::operation_not_permitted, "shared_lock::unlock: the lock is not owned");
    pm_->unlock_shared();
    owns_ = false;
  }

  void swap(shared_lock& u) noexcept {
    Mutex* m = pm_;
    pm_ = u.pm_;
    u.pm_ = m;
    bool o = owns_;
    owns_ = u.owns_;
    u.owns_ = o;
  }
  mutex_type* release() noexcept {
    Mutex* m = pm_;
    pm_ = nullptr;
    owns_ = false;
    return m;
  }

  [[nodiscard]] bool owns_lock() const noexcept { return owns_; }
  explicit operator bool() const noexcept { return owns_; }
  [[nodiscard]] mutex_type* mutex() const noexcept { return pm_; }
};

template <class Mutex>
void swap(shared_lock<Mutex>& x, shared_lock<Mutex>& y) noexcept {
  x.swap(y);
}

} // namespace std
