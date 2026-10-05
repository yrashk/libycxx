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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// ---- the hosted runtime (src/hosted/thread.cpp) ------------------------------------------------
// Throws system_error(error_code(ev, generic_category()), what).
[[noreturn]] void throw_system_error(int ev, const char* what);
// The timed form of atomic_wait_block: false when it returned because the deadline passed.
bool atomic_wait_block_until(const volatile void* addr, std::uint32_t ticket, int clock, long long sec,
                             long long nsec) noexcept;

// Inside a handler: whether the exception being handled is not a C++ exception, such as the
// forced unwind of thread cancellation or pthread_exit, which a catch (...) must rethrow.
bool handling_foreign_exception() noexcept;

[[noreturn]] [[gnu::cold]] inline void raise_system_error(std::errc e, const char* what) {
  if constexpr (cfg::exceptions)
    ::ycxx::detail::throw_system_error(static_cast<int>(e), what);
  else
    ::ycxx_error_handler(ycxx_error_system_error, what);
}

// ---- deadlines ---------------------------------------------------------------------------------
// An absolute time on one of the PAL's clocks.
struct pal_deadline {
  int clock;
  long long sec;
  long long nsec;
};

// Durations are clamped to about 100 years, so that a wait "forever" (duration::max()) neither
// overflows nanoseconds nor wraps around.
inline constexpr long long wait_limit_ns = 100LL * 365 * 24 * 3600 * 1'000'000'000;

// rel in nanoseconds, rounded up, clamped to [0, wait_limit_ns].
template <class Rep, class Period>
constexpr long long clamped_ns(const std::chrono::duration<Rep, Period>& rel) {
  using namespace std::chrono;
  if (!(rel > duration<Rep, Period>::zero()))
    return 0;
  if (!(rel < duration<long double>(static_cast<long double>(wait_limit_ns) / 1e9L)))
    return wait_limit_ns;
  return std::chrono::ceil<nanoseconds>(duration<long double, std::nano>(rel)).count();
}

constexpr pal_deadline make_deadline(int clock, long long ns) noexcept {
  long long s = ns / 1'000'000'000, n = ns % 1'000'000'000;
  if (n < 0) {
    n += 1'000'000'000;
    --s;
  }
  return {clock, s, n};
}

// The monotonic-clock deadline `rel` from now.
template <class Rep, class Period>
pal_deadline deadline_after(const std::chrono::duration<Rep, Period>& rel) {
  return ::ycxx::detail::make_deadline(ycxx_pal_clock_monotonic, ::ycxx::detail::pal_clock_ns(ycxx_pal_clock_monotonic) +
                                                                     ::ycxx::detail::clamped_ns(rel));
}

// The deadline of abs_time: on the realtime clock for system_clock, on the monotonic clock
// otherwise (for other clocks, the time remaining after `now`, a value of Clock::now() the
// caller has just read; callers re-check Clock::now()).
template <class Clock, class Duration>
pal_deadline deadline_at(const std::chrono::time_point<Clock, Duration>& abs, const typename Clock::time_point& now) {
  using namespace std::chrono;
  if constexpr (std::is_same_v<Clock, system_clock> || std::is_same_v<Clock, steady_clock>) {
    constexpr int clock = std::is_same_v<Clock, system_clock> ? ycxx_pal_clock_realtime : ycxx_pal_clock_monotonic;
    const auto since = abs.time_since_epoch();
    long long ns;
    if (since < duration<long double>(-static_cast<long double>(wait_limit_ns) / 1e9L))
      ns = -wait_limit_ns;
    else if (since > duration<long double>(static_cast<long double>(wait_limit_ns) * 2 / 1e9L))
      ns = wait_limit_ns * 2;
    else
      ns = std::chrono::ceil<nanoseconds>(duration<long double, std::nano>(since)).count();
    return ::ycxx::detail::make_deadline(clock, ns);
  } else {
    return ::ycxx::detail::deadline_after(abs - now);
  }
}

// A relative timeout as a steady_clock time point ([thread.req.timing]/3), rounded up and clamped.
template <class Rep, class Period>
std::chrono::steady_clock::time_point steady_deadline(const std::chrono::duration<Rep, Period>& rel) {
  return std::chrono::steady_clock::now() + std::chrono::nanoseconds(::ycxx::detail::clamped_ns(rel));
}

// ---- the futex mutex ----------------------------------------------------------------------------
class futex_mutex {
  ycxx_pal_u32 state_ = 0;

  [[gnu::noinline]] void lock_slow() noexcept {
    for (int i = 0; i < 100; ++i)
      if (__atomic_load_n(&state_, __ATOMIC_RELAXED) == 0 && try_lock())
        return;
    while (__atomic_exchange_n(&state_, 2, __ATOMIC_ACQUIRE) != 0)
      ::ycxx_pal_wait(&state_, 2);
  }

public:
  constexpr futex_mutex() noexcept = default;
  futex_mutex(const futex_mutex&) = delete;
  futex_mutex& operator=(const futex_mutex&) = delete;

  // In a single-threaded process (single_threaded.hpp) nobody can contend or wait, so the state
  // is read and written plainly: a timed lock attempt on a mutex the thread already holds may
  // have left it at 2, which the plain unlock clears without a wake.
  bool try_lock() noexcept {
    if (::ycxx::detail::single_threaded()) {
      if (state_ != 0)
        return false;
      state_ = 1;
      return true;
    }
    ycxx_pal_u32 e = 0;
    return __atomic_compare_exchange_n(&state_, &e, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
  }
  void lock() noexcept {
    if (!try_lock())
      lock_slow();
  }
  // Locks unless the deadline passes first.
  bool lock_until(const pal_deadline& d) noexcept {
    if (try_lock())
      return true;
    for (;;) {
      if (__atomic_exchange_n(&state_, 2, __ATOMIC_ACQUIRE) == 0)
        return true;
      if (::ycxx_pal_wait_until(&state_, 2, d.clock, d.sec, d.nsec) != 0) {
        ycxx_pal_u32 e = 0;
        return __atomic_compare_exchange_n(&state_, &e, 2, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
      }
    }
  }
  void unlock() noexcept {
    if (::ycxx::detail::single_threaded()) {
      state_ = 0;
      return;
    }
    if (__atomic_exchange_n(&state_, 0, __ATOMIC_RELEASE) == 2)
      ::ycxx_pal_wake_one(&state_);
  }
};

// Runs try_once() until it succeeds or abs_time (of any clock) passes, blocking in between with
// block(deadline). Returns the result of the last try_once().
template <class Clock, class Duration, class Try, class Block>
bool try_until(const std::chrono::time_point<Clock, Duration>& abs, Try try_once, Block block) {
  for (;;) {
    if (try_once())
      return true;
    const auto now = Clock::now();
    if (!(now < abs))
      return try_once();
    block(::ycxx::detail::deadline_at(abs, now));
  }
}

// atomic_wait_until_done with a deadline on any clock: false if abs passed with done() false.
template <class Clock, class Duration, class Done>
bool atomic_wait_until_done_by(const volatile void* addr, Done done, const std::chrono::time_point<Clock, Duration>& abs) {
  for (int i = 0; i < 32; ++i)
    if (done())
      return true;
  for (;;) {
    // Computed before registering: Clock::now() may throw, and must not leave a registration.
    const auto now = Clock::now();
    if (!(now < abs))
      return done();
    const pal_deadline d = ::ycxx::detail::deadline_at(abs, now);
    const std::uint32_t ticket = ::ycxx::detail::atomic_wait_prepare(addr);
    if (done()) {
      ::ycxx::detail::atomic_wait_cancel(addr);
      return true;
    }
    ::ycxx::detail::atomic_wait_block_until(addr, ticket, d.clock, d.sec, d.nsec);
    if (done())
      return true;
  }
}

inline ycxx_pal_handle this_thread_handle() noexcept { return ::ycxx_pal_thread_self(); }

}} // namespace ycxx::detail
