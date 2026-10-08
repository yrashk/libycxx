// libycxx hosted: mutual exclusion ([thread.mutex]): the four mutex types, the lock tags,
// lock_guard, scoped_lock, unique_lock, lock / try_lock, once_flag and call_once.
//
// The mutexes are futex locks (__ycxx::__detail::__futex_mutex, thread_support.hpp): constexpr
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

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// A futex mutex that can also be owned recursively by one thread.
class __recursive_futex_mutex {
  __futex_mutex __m_;
  ycxx_pal_handle __owner_ = 0; // read by other threads (atomically), written by the owner
  unsigned long __count_ = 0;

  bool __owned_by_caller() const noexcept { return __atomic_load_n(&__owner_, __ATOMIC_RELAXED) == ::ycxx_pal_thread_self(); }
  void take() noexcept {
    __atomic_store_n(&__owner_, ::ycxx_pal_thread_self(), __ATOMIC_RELAXED);
    __count_ = 1;
  }

public:
  constexpr __recursive_futex_mutex() noexcept = default;

  // 0: acquired anew or recursively, 1: busy, 2: the recursion count would overflow.
  int __try_lock_status() noexcept {
    if (__owned_by_caller()) {
      if (__count_ == static_cast<unsigned long>(-1))
        return 2;
      ++__count_;
      return 0;
    }
    if (!__m_.try_lock())
      return 1;
    take();
    return 0;
  }
  void lock() {
    if (__owned_by_caller()) {
      if (__count_ == static_cast<unsigned long>(-1))
        ::__ycxx::__detail::__raise_system_error(std::errc::resource_unavailable_try_again,
                                           "recursive_mutex::lock: too many levels of ownership");
      ++__count_;
      return;
    }
    __m_.lock();
    take();
  }
  bool try_lock() noexcept { return __try_lock_status() == 0; }
  bool __lock_until(const __pal_deadline& d) noexcept {
    int s = __try_lock_status();
    if (s != 1)
      return s == 0;
    if (!__m_.__lock_until(d))
      return false;
    take();
    return true;
  }
  void unlock() noexcept {
    if (--__count_ == 0) {
      __atomic_store_n(&__owner_, ycxx_pal_handle{0}, __ATOMIC_RELAXED);
      __m_.unlock();
    }
  }
};

template <class _Mp, class _Clock, class _Duration>
bool __timed_try_lock_until(_Mp& m, const std::chrono::time_point<_Clock, _Duration>& abs) {
  static_assert(std::chrono::is_clock_v<_Clock>, "try_lock_until: Clock must meet the Cpp17Clock requirements");
  for (;;) {
    if (m.try_lock())
      return true;
    const auto now = _Clock::now();
    if (!(now < abs))
      return false;
    if (m.__lock_until(::__ycxx::__detail::__deadline_at(abs, now)))
      return true;
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [thread.mutex.class]
class mutex {
  __ycxx::__detail::__futex_mutex __m_;

public:
  constexpr mutex() noexcept = default;
  ~mutex() = default;
  mutex(const mutex&) = delete;
  mutex& operator=(const mutex&) = delete;

  void lock() { __m_.lock(); }
  [[nodiscard]] bool try_lock() noexcept { return __m_.try_lock(); }
  void unlock() noexcept { __m_.unlock(); }
};

// [thread.mutex.recursive]
class recursive_mutex {
  __ycxx::__detail::__recursive_futex_mutex __m_;

public:
  recursive_mutex() noexcept = default;
  ~recursive_mutex() = default;
  recursive_mutex(const recursive_mutex&) = delete;
  recursive_mutex& operator=(const recursive_mutex&) = delete;

  void lock() { __m_.lock(); }
  [[nodiscard]] bool try_lock() noexcept { return __m_.try_lock(); }
  void unlock() noexcept { __m_.unlock(); }
};

// [thread.timedmutex.class]
class timed_mutex {
  __ycxx::__detail::__futex_mutex __m_;

public:
  timed_mutex() noexcept = default;
  ~timed_mutex() = default;
  timed_mutex(const timed_mutex&) = delete;
  timed_mutex& operator=(const timed_mutex&) = delete;

  void lock() { __m_.lock(); }
  [[nodiscard]] bool try_lock() noexcept { return __m_.try_lock(); }
  template <class _Rep, class _Period>
  [[nodiscard]] bool try_lock_for(const chrono::duration<_Rep, _Period>& __rel_time) {
    return try_lock_until(__ycxx::__detail::__steady_deadline(__rel_time));
  }
  template <class _Clock, class _Duration>
  [[nodiscard]] bool try_lock_until(const chrono::time_point<_Clock, _Duration>& __abs_time) {
    return __ycxx::__detail::__timed_try_lock_until(__m_, __abs_time);
  }
  void unlock() noexcept { __m_.unlock(); }
};

// [thread.timedmutex.recursive]
class recursive_timed_mutex {
  __ycxx::__detail::__recursive_futex_mutex __m_;

public:
  recursive_timed_mutex() noexcept = default;
  ~recursive_timed_mutex() = default;
  recursive_timed_mutex(const recursive_timed_mutex&) = delete;
  recursive_timed_mutex& operator=(const recursive_timed_mutex&) = delete;

  void lock() { __m_.lock(); }
  [[nodiscard]] bool try_lock() noexcept { return __m_.try_lock(); }
  template <class _Rep, class _Period>
  [[nodiscard]] bool try_lock_for(const chrono::duration<_Rep, _Period>& __rel_time) {
    return try_lock_until(__ycxx::__detail::__steady_deadline(__rel_time));
  }
  template <class _Clock, class _Duration>
  [[nodiscard]] bool try_lock_until(const chrono::time_point<_Clock, _Duration>& __abs_time) {
    return __ycxx::__detail::__timed_try_lock_until(__m_, __abs_time);
  }
  void unlock() noexcept { __m_.unlock(); }
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
template <class _Mutex>
class lock_guard {
  _Mutex& __pm_;

public:
  using mutex_type = _Mutex;
  explicit lock_guard(mutex_type& m) : __pm_(m) { m.lock(); }
  lock_guard(mutex_type& m, adopt_lock_t) : __pm_(m) {}
  ~lock_guard() { __pm_.unlock(); }
  lock_guard(const lock_guard&) = delete;
  lock_guard& operator=(const lock_guard&) = delete;
};

// [thread.lock.algorithm]
template <class _L1, class _L2, class... _L3>
[[nodiscard]] int try_lock(_L1& __l1, _L2& __l2, _L3&... __l3);
template <class _L1, class _L2, class... _L3>
void lock(_L1& __l1, _L2& __l2, _L3&... __l3);

// [thread.lock.scoped]
template <class... _MutexTypes>
class scoped_lock {
  tuple<_MutexTypes&...> __pm_;

public:
  explicit scoped_lock(_MutexTypes&... m) : __pm_(m...) { std::lock(m...); }
  explicit scoped_lock(adopt_lock_t, _MutexTypes&... m) : __pm_(m...) {}
  ~scoped_lock() {
    [this]<size_t... _Ip>(index_sequence<_Ip...>) { (std::get<_Ip>(__pm_).unlock(), ...); }(index_sequence_for<_MutexTypes...>{});
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
template <class _Mutex>
class scoped_lock<_Mutex> {
  _Mutex& __pm_;

public:
  using mutex_type = _Mutex;
  explicit scoped_lock(_Mutex& m) : __pm_(m) { m.lock(); }
  explicit scoped_lock(adopt_lock_t, _Mutex& m) : __pm_(m) {}
  ~scoped_lock() { __pm_.unlock(); }
  scoped_lock(const scoped_lock&) = delete;
  scoped_lock& operator=(const scoped_lock&) = delete;
};

// [thread.lock.unique]
template <class _Mutex>
class unique_lock {
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

  unique_lock() noexcept = default;
  explicit unique_lock(mutex_type& m) : __pm_(__builtin_addressof(m)) {
    m.lock();
    __owns_ = true;
  }
  unique_lock(mutex_type& m, defer_lock_t) noexcept : __pm_(__builtin_addressof(m)) {}
  unique_lock(mutex_type& m, try_to_lock_t) : __pm_(__builtin_addressof(m)), __owns_(m.try_lock()) {}
  unique_lock(mutex_type& m, adopt_lock_t) : __pm_(__builtin_addressof(m)), __owns_(true) {}
  template <class _Clock, class _Duration>
  unique_lock(mutex_type& m, const chrono::time_point<_Clock, _Duration>& __abs_time)
      : __pm_(__builtin_addressof(m)), __owns_(m.try_lock_until(__abs_time)) {}
  template <class _Rep, class _Period>
  unique_lock(mutex_type& m, const chrono::duration<_Rep, _Period>& __rel_time)
      : __pm_(__builtin_addressof(m)), __owns_(m.try_lock_for(__rel_time)) {}
  ~unique_lock() {
    if (__owns_)
      __pm_->unlock();
  }
  unique_lock(const unique_lock&) = delete;
  unique_lock& operator=(const unique_lock&) = delete;
  unique_lock(unique_lock&& __u) noexcept : __pm_(__u.__pm_), __owns_(__u.__owns_) {
    __u.__pm_ = nullptr;
    __u.__owns_ = false;
  }
  unique_lock& operator=(unique_lock&& __u) noexcept {
    unique_lock(static_cast<unique_lock&&>(__u)).swap(*this);
    return *this;
  }

  void lock() {
    __check_lockable("unique_lock::lock");
    __pm_->lock();
    __owns_ = true;
  }
  [[nodiscard]] bool try_lock() {
    __check_lockable("unique_lock::try_lock");
    __owns_ = __pm_->try_lock();
    return __owns_;
  }
  template <class _Rep, class _Period>
  [[nodiscard]] bool try_lock_for(const chrono::duration<_Rep, _Period>& __rel_time) {
    __check_lockable("unique_lock::try_lock_for");
    __owns_ = __pm_->try_lock_for(__rel_time);
    return __owns_;
  }
  template <class _Clock, class _Duration>
  [[nodiscard]] bool try_lock_until(const chrono::time_point<_Clock, _Duration>& __abs_time) {
    __check_lockable("unique_lock::try_lock_until");
    __owns_ = __pm_->try_lock_until(__abs_time);
    return __owns_;
  }
  void unlock() {
    if (!__owns_)
      __ycxx::__detail::__raise_system_error(errc::operation_not_permitted, "unique_lock::unlock: the lock is not owned");
    __pm_->unlock();
    __owns_ = false;
  }

  void swap(unique_lock& __u) noexcept {
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
void swap(unique_lock<_Mutex>& __x, unique_lock<_Mutex>& y) noexcept {
  __x.swap(y);
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// The lockables of lock/try_lock behind type-erased thunks, so they can be indexed at run time.
// lock() is used only by std::lock (`_Lock`): std::try_lock needs only try_lock and unlock
// (Cpp17Lockable without the blocking lock is not required there).
template <std::size_t _Np, bool _Lock>
struct __lockable_set {
  void* __obj[_Np];
  void (*__lock_fn[_Np])(void*);
  bool (*__try_lock_fn[_Np])(void*);
  void (*__unlock_fn[_Np])(void*);

  template <class _Lp>
  static void __lock_one(void* p) {
    if constexpr (_Lock)
      static_cast<_Lp*>(p)->lock();
  }

  template <class... _Lp>
  explicit __lockable_set(_Lp&... __l) noexcept
      : __obj{static_cast<void*>(__builtin_addressof(__l))...}, __lock_fn{&__lock_one<_Lp>...},
        __try_lock_fn{[](void* p) -> bool { return static_cast<_Lp*>(p)->try_lock(); }...},
        __unlock_fn{[](void* p) { static_cast<_Lp*>(p)->unlock(); }...} {}

  // Unlocks `count` lockables starting at index `first` (rotating) when destroyed.
  struct __release_guard {
    __lockable_set* set;
    std::size_t first;
    std::size_t count;
    ~__release_guard() {
      for (std::size_t k = 0; k < count; ++k) {
        const std::size_t __j = (first + k) % _Np;
        set->__unlock_fn[__j](set->__obj[__j]);
      }
    }
  };
};

// [thread.lock.algorithm]/1-3: try_lock each in order; -1 when all succeed, else the 0-based
// index of the first failure, the earlier ones unlocked again.
template <class... _Lp>
int __try_lock_all(_Lp&... __l) {
  constexpr std::size_t n = sizeof...(_Lp);
  __lockable_set<n, false> set(__l...);
  typename __lockable_set<n, false>::__release_guard __g{&set, 0, 0};
  for (std::size_t i = 0; i < n; ++i) {
    if (!set.__try_lock_fn[i](set.__obj[i]))
      return static_cast<int>(i);
    ++__g.count;
  }
  __g.count = 0;
  return -1;
}

// [thread.lock.algorithm]/4-5: deadlock avoidance by blocking on one lockable and trying the
// others; when one is busy, everything is released and the next round blocks on the busy one.
template <class... _Lp>
void __lock_all(_Lp&... __l) {
  constexpr std::size_t n = sizeof...(_Lp);
  __lockable_set<n, true> set(__l...);
  std::size_t first = 0;
  for (;;) {
    set.__lock_fn[first](set.__obj[first]);
    typename __lockable_set<n, true>::__release_guard __g{&set, first, 1};
    std::size_t __busy = n;
    for (std::size_t k = 1; k < n; ++k) {
      const std::size_t __j = (first + k) % n;
      if (!set.__try_lock_fn[__j](set.__obj[__j])) {
        __busy = __j;
        break;
      }
      ++__g.count;
    }
    if (__busy == n) {
      __g.count = 0;
      return;
    }
    first = __busy;
    // g releases this round's locks; give the holder of the busy one a chance first.
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _L1, class _L2, class... _L3>
int try_lock(_L1& __l1, _L2& __l2, _L3&... __l3) {
  return __ycxx::__detail::__try_lock_all(__l1, __l2, __l3...);
}

template <class _L1, class _L2, class... _L3>
void lock(_L1& __l1, _L2& __l2, _L3&... __l3) {
  __ycxx::__detail::__lock_all(__l1, __l2, __l3...);
}

// [thread.once]
struct once_flag {
  constexpr once_flag() noexcept = default;
  once_flag(const once_flag&) = delete;
  once_flag& operator=(const once_flag&) = delete;

private:
  ycxx_pal_u32 __state_ = 0; // 0: not run, 1: running, 2: running with waiters, 3: done
  template <class _Callable, class... _Args>
  friend void call_once(once_flag& __flag, _Callable&& __func, _Args&&... __args);
};

template <class _Callable, class... _Args>
void call_once(once_flag& __flag, _Callable&& __func, _Args&&... __args) {
  ycxx_pal_u32* s = &__flag.__state_;
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
      __ycxx::__detail::invoke(static_cast<_Callable&&>(__func), static_cast<_Args&&>(__args)...);
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

}} // namespace std
