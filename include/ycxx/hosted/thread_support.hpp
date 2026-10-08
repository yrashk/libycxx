// libycxx hosted: the pieces shared by the thread support library ([thread]): error reporting,
// deadlines for the PAL's timed waits, the futex-based mutex every lock type is built on, and
// timed waits on the atomic slot table.
//
// Mutexes and condition variables are built directly on the PAL's address wait
// (ycxx_pal_wait / ycxx_pal_wake_* / ycxx_pal_wait_until; a futex on Linux), not on pthread
// mutexes: they are constexpr-constructible, need no destruction, and the PAL stays small. The
// mutex is the classic three-state futex lock (0 free, 1 held, 2 held and maybe waited for).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/atomic_base.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/core/errc.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/single_threaded.hpp>
#include <ycxx/hosted/chrono_clocks.hpp>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// ---- the hosted runtime (src/hosted/thread.cpp) ------------------------------------------------
// Throws system_error(error_code(ev, generic_category()), what).
[[noreturn]] void __throw_system_error(int __ev, const char* what);
// The timed form of atomic_wait_block: false when it returned because the deadline passed.
bool __atomic_wait_block_until(const volatile void* __addr, std::uint32_t __ticket, int clock, long long __sec,
                             long long __nsec) noexcept;

// Inside a handler: whether the exception being handled is not a C++ exception, such as the
// forced unwind of thread cancellation or pthread_exit, which a catch (...) must rethrow.
bool __handling_foreign_exception() noexcept;

[[noreturn]] [[__gnu__::__cold__]] inline void __raise_system_error(std::errc e, const char* what) {
  if constexpr (__cfg::exceptions)
    ::__ycxx::__detail::__throw_system_error(static_cast<int>(e), what);
  else
    ::ycxx_error_handler(ycxx_error_system_error, what);
}

// ---- deadlines ---------------------------------------------------------------------------------
// An absolute time on one of the PAL's clocks.
struct __pal_deadline {
  int clock;
  long long __sec;
  long long __nsec;
};

// Durations are clamped to about 100 years, so that a wait "forever" (duration::max()) neither
// overflows nanoseconds nor wraps around.
inline constexpr long long __wait_limit_ns = 100LL * 365 * 24 * 3600 * 1'000'000'000;

// rel in nanoseconds, rounded up, clamped to [0, wait_limit_ns].
template <class _Rep, class _Period>
constexpr long long __clamped_ns(const std::chrono::duration<_Rep, _Period>& __rel) {
  using namespace std::chrono;
  if (!(__rel > duration<_Rep, _Period>::zero()))
    return 0;
  if (!(__rel < duration<long double>(static_cast<long double>(__wait_limit_ns) / 1e9L)))
    return __wait_limit_ns;
  return std::chrono::ceil<nanoseconds>(duration<long double, std::nano>(__rel)).count();
}

constexpr __pal_deadline __make_deadline(int clock, long long ns) noexcept {
  long long s = ns / 1'000'000'000, n = ns % 1'000'000'000;
  if (n < 0) {
    n += 1'000'000'000;
    --s;
  }
  return {clock, s, n};
}

// The monotonic-clock deadline `__rel` from now.
template <class _Rep, class _Period>
__pal_deadline __deadline_after(const std::chrono::duration<_Rep, _Period>& __rel) {
  return ::__ycxx::__detail::__make_deadline(ycxx_pal_clock_monotonic, ::__ycxx::__detail::__pal_clock_ns(ycxx_pal_clock_monotonic) +
                                                                     ::__ycxx::__detail::__clamped_ns(__rel));
}

// The deadline of abs_time: on the realtime clock for system_clock, on the monotonic clock
// otherwise (for other clocks, the time remaining after `now`, a value of Clock::now() the
// caller has just read; callers re-check Clock::now()).
template <class _Clock, class _Duration>
__pal_deadline __deadline_at(const std::chrono::time_point<_Clock, _Duration>& abs, const typename _Clock::time_point& now) {
  using namespace std::chrono;
  if constexpr (std::is_same_v<_Clock, system_clock> || std::is_same_v<_Clock, steady_clock>) {
    constexpr int clock = std::is_same_v<_Clock, system_clock> ? ycxx_pal_clock_realtime : ycxx_pal_clock_monotonic;
    const auto __since = abs.time_since_epoch();
    long long ns;
    if (__since < duration<long double>(-static_cast<long double>(__wait_limit_ns) / 1e9L))
      ns = -__wait_limit_ns;
    else if (__since > duration<long double>(static_cast<long double>(__wait_limit_ns) * 2 / 1e9L))
      ns = __wait_limit_ns * 2;
    else
      ns = std::chrono::ceil<nanoseconds>(duration<long double, std::nano>(__since)).count();
    return ::__ycxx::__detail::__make_deadline(clock, ns);
  } else {
    return ::__ycxx::__detail::__deadline_after(abs - now);
  }
}

// A relative timeout as a steady_clock time point ([thread.req.timing]/3), rounded up and clamped.
template <class _Rep, class _Period>
std::chrono::steady_clock::time_point __steady_deadline(const std::chrono::duration<_Rep, _Period>& __rel) {
  return std::chrono::steady_clock::now() + std::chrono::nanoseconds(::__ycxx::__detail::__clamped_ns(__rel));
}

// ---- the futex mutex ----------------------------------------------------------------------------
class __futex_mutex {
  ycxx_pal_u32 __state_ = 0;

  [[__gnu__::__noinline__]] void __lock_slow() noexcept {
    for (int i = 0; i < 100; ++i)
      if (__atomic_load_n(&__state_, __ATOMIC_RELAXED) == 0 && try_lock())
        return;
    while (__atomic_exchange_n(&__state_, 2, __ATOMIC_ACQUIRE) != 0)
      ::ycxx_pal_wait(&__state_, 2);
  }

public:
  constexpr __futex_mutex() noexcept = default;
  __futex_mutex(const __futex_mutex&) = delete;
  __futex_mutex& operator=(const __futex_mutex&) = delete;

  // In a single-threaded process (single_threaded.hpp) nobody can contend or wait, so the state
  // is read and written plainly: a timed lock attempt on a mutex the thread already holds may
  // have left it at 2, which the plain unlock clears without a wake.
  bool try_lock() noexcept {
    if (::__ycxx::__detail::__single_threaded()) {
      if (__state_ != 0)
        return false;
      __state_ = 1;
      return true;
    }
    ycxx_pal_u32 e = 0;
    return __atomic_compare_exchange_n(&__state_, &e, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
  }
  void lock() noexcept {
    if (!try_lock())
      __lock_slow();
  }
  // Locks unless the deadline passes first.
  bool __lock_until(const __pal_deadline& d) noexcept {
    if (try_lock())
      return true;
    for (;;) {
      if (__atomic_exchange_n(&__state_, 2, __ATOMIC_ACQUIRE) == 0)
        return true;
      if (::ycxx_pal_wait_until(&__state_, 2, d.clock, d.__sec, d.__nsec) != 0) {
        ycxx_pal_u32 e = 0;
        return __atomic_compare_exchange_n(&__state_, &e, 2, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
      }
    }
  }
  void unlock() noexcept {
    if (::__ycxx::__detail::__single_threaded()) {
      __state_ = 0;
      return;
    }
    if (__atomic_exchange_n(&__state_, 0, __ATOMIC_RELEASE) == 2)
      ::ycxx_pal_wake_one(&__state_);
  }
};

// Runs try_once() until it succeeds or abs_time (of any clock) passes, blocking in between with
// block(deadline). Returns the result of the last try_once().
template <class _Clock, class _Duration, class _Try, class _Block>
bool __try_until(const std::chrono::time_point<_Clock, _Duration>& abs, _Try __try_once, _Block block) {
  for (;;) {
    if (__try_once())
      return true;
    const auto now = _Clock::now();
    if (!(now < abs))
      return __try_once();
    block(::__ycxx::__detail::__deadline_at(abs, now));
  }
}

// atomic_wait_until_done with a deadline on any clock: false if abs passed with done() false.
template <class _Clock, class _Duration, class _Done>
bool __atomic_wait_until_done_by(const volatile void* __addr, _Done done, const std::chrono::time_point<_Clock, _Duration>& abs) {
  for (int i = 0; i < 32; ++i)
    if (done())
      return true;
  for (;;) {
    // Computed before registering: Clock::now() may throw, and must not leave a registration.
    const auto now = _Clock::now();
    if (!(now < abs))
      return done();
    const __pal_deadline d = ::__ycxx::__detail::__deadline_at(abs, now);
    const std::uint32_t __ticket = ::__ycxx::__detail::__atomic_wait_prepare(__addr);
    if (done()) {
      ::__ycxx::__detail::__atomic_wait_cancel(__addr);
      return true;
    }
    ::__ycxx::__detail::__atomic_wait_block_until(__addr, __ticket, d.clock, d.__sec, d.__nsec);
    if (done())
      return true;
  }
}

inline ycxx_pal_handle __this_thread_handle() noexcept { return ::ycxx_pal_thread_self(); }

}} // namespace __ycxx::__detail
