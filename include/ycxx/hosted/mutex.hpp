// libycxx hosted: mutual exclusion ([thread.mutex]): the four mutex types, the lock tags,
// lock_guard, scoped_lock, unique_lock, lock / try_lock, once_flag and call_once.
//
// The mutexes are futex locks (ycxx::detail::futex_mutex, thread_support.hpp): constexpr
// constructible and trivially destructible. A recursive mutex records its owner's PAL thread
// handle. call_once keeps a three-state word (not run, running, done) plus a "waiters" state,
// and blocks on it through the PAL. No native_handle members are provided ([thread.req.native]).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/core/integer_sequence.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/hosted/chrono_clocks.hpp>
#include <ycxx/hosted/thread_support.hpp>
#include <ycxx/pal.h>

namespace ycxx::detail {

// A futex mutex that can also be owned recursively by one thread.
class recursive_futex_mutex {
  futex_mutex m_;
  ycxx_pal_handle owner_ = 0; // read by other threads (atomically), written by the owner
  unsigned long count_ = 0;

  bool owned_by_caller() const noexcept { return __atomic_load_n(&owner_, __ATOMIC_RELAXED) == ::ycxx_pal_thread_self(); }
  void take() noexcept {
    __atomic_store_n(&owner_, ::ycxx_pal_thread_self(), __ATOMIC_RELAXED);
    count_ = 1;
  }

public:
  constexpr recursive_futex_mutex() noexcept = default;

  // 0: acquired anew or recursively, 1: busy, 2: the recursion count would overflow.
  int try_lock_status() noexcept {
    if (owned_by_caller()) {
      if (count_ == static_cast<unsigned long>(-1))
        return 2;
      ++count_;
      return 0;
    }
    if (!m_.try_lock())
      return 1;
    take();
    return 0;
  }
  void lock() {
    if (owned_by_caller()) {
      if (count_ == static_cast<unsigned long>(-1))
        ::ycxx::detail::raise_system_error(std::errc::resource_unavailable_try_again,
                                           "recursive_mutex::lock: too many levels of ownership");
      ++count_;
      return;
    }
    m_.lock();
    take();
  }
  bool try_lock() noexcept { return try_lock_status() == 0; }
  bool lock_until(const pal_deadline& d) noexcept {
    int s = try_lock_status();
    if (s != 1)
      return s == 0;
    if (!m_.lock_until(d))
      return false;
    take();
    return true;
  }
  void unlock() noexcept {
    if (--count_ == 0) {
      __atomic_store_n(&owner_, ycxx_pal_handle{0}, __ATOMIC_RELAXED);
      m_.unlock();
    }
  }
};

template <class M, class Clock, class Duration>
bool timed_try_lock_until(M& m, const std::chrono::time_point<Clock, Duration>& abs) {
  static_assert(std::chrono::is_clock_v<Clock>, "try_lock_until: Clock must meet the Cpp17Clock requirements");
  for (;;) {
    if (m.try_lock())
      return true;
    if (!(Clock::now() < abs))
      return false;
    if (m.lock_until(::ycxx::detail::deadline_at(abs)))
      return true;
  }
}

} // namespace ycxx::detail

namespace std {

// [thread.mutex.class]
class mutex {
  ycxx::detail::futex_mutex m_;

public:
  constexpr mutex() noexcept = default;
  ~mutex() = default;
  mutex(const mutex&) = delete;
  mutex& operator=(const mutex&) = delete;

  void lock() { m_.lock(); }
  [[nodiscard]] bool try_lock() noexcept { return m_.try_lock(); }
  void unlock() noexcept { m_.unlock(); }
};

// [thread.mutex.recursive]
class recursive_mutex {
  ycxx::detail::recursive_futex_mutex m_;

public:
  constexpr recursive_mutex() noexcept = default;
  ~recursive_mutex() = default;
  recursive_mutex(const recursive_mutex&) = delete;
  recursive_mutex& operator=(const recursive_mutex&) = delete;

  void lock() { m_.lock(); }
  [[nodiscard]] bool try_lock() noexcept { return m_.try_lock(); }
  void unlock() noexcept { m_.unlock(); }
};

// [thread.timedmutex.class]
class timed_mutex {
  ycxx::detail::futex_mutex m_;

public:
  constexpr timed_mutex() noexcept = default;
  ~timed_mutex() = default;
  timed_mutex(const timed_mutex&) = delete;
  timed_mutex& operator=(const timed_mutex&) = delete;

  void lock() { m_.lock(); }
  [[nodiscard]] bool try_lock() noexcept { return m_.try_lock(); }
  template <class Rep, class Period>
  [[nodiscard]] bool try_lock_for(const chrono::duration<Rep, Period>& rel_time) {
    return try_lock_until(ycxx::detail::steady_deadline(rel_time));
  }
  template <class Clock, class Duration>
  [[nodiscard]] bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time) {
    return ycxx::detail::timed_try_lock_until(m_, abs_time);
  }
  void unlock() noexcept { m_.unlock(); }
};

// [thread.timedmutex.recursive]
class recursive_timed_mutex {
  ycxx::detail::recursive_futex_mutex m_;

public:
  constexpr recursive_timed_mutex() noexcept = default;
  ~recursive_timed_mutex() = default;
  recursive_timed_mutex(const recursive_timed_mutex&) = delete;
  recursive_timed_mutex& operator=(const recursive_timed_mutex&) = delete;

  void lock() { m_.lock(); }
  [[nodiscard]] bool try_lock() noexcept { return m_.try_lock(); }
  template <class Rep, class Period>
  [[nodiscard]] bool try_lock_for(const chrono::duration<Rep, Period>& rel_time) {
    return try_lock_until(ycxx::detail::steady_deadline(rel_time));
  }
  template <class Clock, class Duration>
  [[nodiscard]] bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time) {
    return ycxx::detail::timed_try_lock_until(m_, abs_time);
  }
  void unlock() noexcept { m_.unlock(); }
};

struct defer_lock_t {
  explicit defer_lock_t() = default;
};
struct try_to_lock_t {
  explicit try_to_lock_t() = default;
};
struct adopt_lock_t {
  explicit adopt_lock_t() = default;
};
inline constexpr defer_lock_t defer_lock{};
inline constexpr try_to_lock_t try_to_lock{};
inline constexpr adopt_lock_t adopt_lock{};

// [thread.lock.guard]
template <class Mutex>
class lock_guard {
  Mutex& pm_;

public:
  using mutex_type = Mutex;
  explicit lock_guard(mutex_type& m) : pm_(m) { m.lock(); }
  lock_guard(mutex_type& m, adopt_lock_t) : pm_(m) {}
  ~lock_guard() { pm_.unlock(); }
  lock_guard(const lock_guard&) = delete;
  lock_guard& operator=(const lock_guard&) = delete;
};

// [thread.lock.algorithm]
template <class L1, class L2, class... L3>
[[nodiscard]] int try_lock(L1& l1, L2& l2, L3&... l3);
template <class L1, class L2, class... L3>
void lock(L1& l1, L2& l2, L3&... l3);

// [thread.lock.scoped]
template <class... MutexTypes>
class scoped_lock {
  tuple<MutexTypes&...> pm_;

public:
  explicit scoped_lock(MutexTypes&... m) : pm_(m...) { std::lock(m...); }
  explicit scoped_lock(adopt_lock_t, MutexTypes&... m) : pm_(m...) {}
  ~scoped_lock() {
    [this]<size_t... I>(index_sequence<I...>) { (std::get<I>(pm_).unlock(), ...); }(index_sequence_for<MutexTypes...>{});
  }
  scoped_lock(const scoped_lock&) = delete;
  scoped_lock& operator=(const scoped_lock&) = delete;
};
template <>
class scoped_lock<> {
public:
  explicit scoped_lock() = default;
  explicit scoped_lock(adopt_lock_t) {}
  scoped_lock(const scoped_lock&) = delete;
  scoped_lock& operator=(const scoped_lock&) = delete;
};
template <class Mutex>
class scoped_lock<Mutex> {
  Mutex& pm_;

public:
  using mutex_type = Mutex;
  explicit scoped_lock(Mutex& m) : pm_(m) { m.lock(); }
  explicit scoped_lock(adopt_lock_t, Mutex& m) : pm_(m) {}
  ~scoped_lock() { pm_.unlock(); }
  scoped_lock(const scoped_lock&) = delete;
  scoped_lock& operator=(const scoped_lock&) = delete;
};

// [thread.lock.unique]
template <class Mutex>
class unique_lock {
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

  unique_lock() noexcept = default;
  explicit unique_lock(mutex_type& m) : pm_(__builtin_addressof(m)) {
    m.lock();
    owns_ = true;
  }
  unique_lock(mutex_type& m, defer_lock_t) noexcept : pm_(__builtin_addressof(m)) {}
  unique_lock(mutex_type& m, try_to_lock_t) : pm_(__builtin_addressof(m)), owns_(m.try_lock()) {}
  unique_lock(mutex_type& m, adopt_lock_t) : pm_(__builtin_addressof(m)), owns_(true) {}
  template <class Clock, class Duration>
  unique_lock(mutex_type& m, const chrono::time_point<Clock, Duration>& abs_time)
      : pm_(__builtin_addressof(m)), owns_(m.try_lock_until(abs_time)) {}
  template <class Rep, class Period>
  unique_lock(mutex_type& m, const chrono::duration<Rep, Period>& rel_time)
      : pm_(__builtin_addressof(m)), owns_(m.try_lock_for(rel_time)) {}
  ~unique_lock() {
    if (owns_)
      pm_->unlock();
  }
  unique_lock(const unique_lock&) = delete;
  unique_lock& operator=(const unique_lock&) = delete;
  unique_lock(unique_lock&& u) noexcept : pm_(u.pm_), owns_(u.owns_) {
    u.pm_ = nullptr;
    u.owns_ = false;
  }
  unique_lock& operator=(unique_lock&& u) noexcept {
    unique_lock(static_cast<unique_lock&&>(u)).swap(*this);
    return *this;
  }

  void lock() {
    check_lockable("unique_lock::lock");
    pm_->lock();
    owns_ = true;
  }
  [[nodiscard]] bool try_lock() {
    check_lockable("unique_lock::try_lock");
    owns_ = pm_->try_lock();
    return owns_;
  }
  template <class Rep, class Period>
  [[nodiscard]] bool try_lock_for(const chrono::duration<Rep, Period>& rel_time) {
    check_lockable("unique_lock::try_lock_for");
    owns_ = pm_->try_lock_for(rel_time);
    return owns_;
  }
  template <class Clock, class Duration>
  [[nodiscard]] bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time) {
    check_lockable("unique_lock::try_lock_until");
    owns_ = pm_->try_lock_until(abs_time);
    return owns_;
  }
  void unlock() {
    if (!owns_)
      ycxx::detail::raise_system_error(errc::operation_not_permitted, "unique_lock::unlock: the lock is not owned");
    pm_->unlock();
    owns_ = false;
  }

  void swap(unique_lock& u) noexcept {
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
void swap(unique_lock<Mutex>& x, unique_lock<Mutex>& y) noexcept {
  x.swap(y);
}

} // namespace std

namespace ycxx::detail {

// The lockables of lock/try_lock behind type-erased thunks, so they can be indexed at run time.
// lock() is used only by std::lock (`Lock`): std::try_lock needs only try_lock and unlock
// (Cpp17Lockable without the blocking lock is not required there).
template <std::size_t N, bool Lock>
struct lockable_set {
  void* obj[N];
  void (*lock_fn[N])(void*);
  bool (*try_lock_fn[N])(void*);
  void (*unlock_fn[N])(void*);

  template <class L>
  static void lock_one(void* p) {
    if constexpr (Lock)
      static_cast<L*>(p)->lock();
  }

  template <class... L>
  explicit lockable_set(L&... l) noexcept
      : obj{static_cast<void*>(__builtin_addressof(l))...}, lock_fn{&lock_one<L>...},
        try_lock_fn{[](void* p) -> bool { return static_cast<L*>(p)->try_lock(); }...},
        unlock_fn{[](void* p) { static_cast<L*>(p)->unlock(); }...} {}

  // Unlocks `count` lockables starting at index `first` (rotating) when destroyed.
  struct release_guard {
    lockable_set* set;
    std::size_t first;
    std::size_t count;
    ~release_guard() {
      for (std::size_t k = 0; k < count; ++k) {
        const std::size_t j = (first + k) % N;
        set->unlock_fn[j](set->obj[j]);
      }
    }
  };
};

// [thread.lock.algorithm]/1-3: try_lock each in order; -1 when all succeed, else the 0-based
// index of the first failure, the earlier ones unlocked again.
template <class... L>
int try_lock_all(L&... l) {
  constexpr std::size_t n = sizeof...(L);
  lockable_set<n, false> set(l...);
  typename lockable_set<n, false>::release_guard g{&set, 0, 0};
  for (std::size_t i = 0; i < n; ++i) {
    if (!set.try_lock_fn[i](set.obj[i]))
      return static_cast<int>(i);
    ++g.count;
  }
  g.count = 0;
  return -1;
}

// [thread.lock.algorithm]/4-5: deadlock avoidance by blocking on one lockable and trying the
// others; when one is busy, everything is released and the next round blocks on the busy one.
template <class... L>
void lock_all(L&... l) {
  constexpr std::size_t n = sizeof...(L);
  lockable_set<n, true> set(l...);
  std::size_t first = 0;
  for (;;) {
    set.lock_fn[first](set.obj[first]);
    typename lockable_set<n, true>::release_guard g{&set, first, 1};
    std::size_t busy = n;
    for (std::size_t k = 1; k < n; ++k) {
      const std::size_t j = (first + k) % n;
      if (!set.try_lock_fn[j](set.obj[j])) {
        busy = j;
        break;
      }
      ++g.count;
    }
    if (busy == n) {
      g.count = 0;
      return;
    }
    first = busy;
    // g releases this round's locks; give the holder of the busy one a chance first.
  }
}

} // namespace ycxx::detail

namespace std {

template <class L1, class L2, class... L3>
int try_lock(L1& l1, L2& l2, L3&... l3) {
  return ycxx::detail::try_lock_all(l1, l2, l3...);
}

template <class L1, class L2, class... L3>
void lock(L1& l1, L2& l2, L3&... l3) {
  ycxx::detail::lock_all(l1, l2, l3...);
}

// [thread.once]
struct once_flag {
  constexpr once_flag() noexcept = default;
  once_flag(const once_flag&) = delete;
  once_flag& operator=(const once_flag&) = delete;

private:
  ycxx_pal_u32 state_ = 0; // 0: not run, 1: running, 2: running with waiters, 3: done
  template <class Callable, class... Args>
  friend void call_once(once_flag& flag, Callable&& func, Args&&... args);
};

template <class Callable, class... Args>
void call_once(once_flag& flag, Callable&& func, Args&&... args) {
  ycxx_pal_u32* s = &flag.state_;
  if (__atomic_load_n(s, __ATOMIC_ACQUIRE) == 3)
    return;
  for (;;) {
    ycxx_pal_u32 cur = 0;
    if (__atomic_compare_exchange_n(s, &cur, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)) {
      // An exceptional execution leaves the flag unset and lets a waiter try.
      struct reset {
        ycxx_pal_u32* s;
        ycxx_pal_u32 to;
        ~reset() {
          if (__atomic_exchange_n(s, to, __ATOMIC_RELEASE) == 2)
            ::ycxx_pal_wake_all(s);
        }
      } r{s, 0};
      ycxx::detail::invoke(static_cast<Callable&&>(func), static_cast<Args&&>(args)...);
      r.to = 3;
      return;
    }
    if (cur == 3)
      return;
    if (cur == 1 && !__atomic_compare_exchange_n(s, &cur, 2, false, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE))
      continue;
    ::ycxx_pal_wait(s, 2);
  }
}

} // namespace std
