// libycxx hosted: the clocks of <chrono> ([time.clock.system], [time.clock.steady],
// [time.clock.hires], [time.clock.file]), on the PAL's __ycxx_pal_clock_now.
//
// All three count nanoseconds in a long long. system_clock is the realtime clock (Unix time),
// steady_clock the monotonic clock, high_resolution_clock a distinct steady clock whose
// time_point is steady_clock's. file_clock has system_clock's epoch.
//
// The clocks need only the 'clock' hosted layer's ycxx_pal_clock_now, so they are also available
// to programs compiled freestanding with that layer (DECISIONS §18); to_time_t and from_time_t,
// whose time_t is the C library's, only with a C library.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/pal.h>
#if YCXX_HOSTED
#  include <time.h>
#endif

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// The current value of a PAL clock in nanoseconds.
inline long long __pal_clock_ns(int clock) noexcept {
  __ycxx_pal_i64 __sec = 0, __nsec = 0;
  ::__ycxx_pal_clock_now(clock, &__sec, &__nsec);
  return static_cast<long long>(__sec) * 1'000'000'000 + static_cast<long long>(__nsec);
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

class system_clock {
public:
  using rep = long long;
  using period = nano;
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<system_clock>;
  static constexpr bool is_steady = false;

  static time_point now() noexcept {
    return time_point(duration(__ycxx::__detail::__pal_clock_ns(__ycxx_pal_clock_realtime)));
  }
#if YCXX_HOSTED
  // Truncated toward negative infinity to whole seconds.
  static ::time_t to_time_t(const time_point& t) noexcept {
    return static_cast<::time_t>(chrono::floor<seconds>(t.time_since_epoch()).count());
  }
  static time_point from_time_t(::time_t t) noexcept {
    return time_point(chrono::duration_cast<duration>(seconds(static_cast<seconds::rep>(t))));
  }
#endif
};

class steady_clock {
public:
  using rep = long long;
  using period = nano;
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<steady_clock, duration>;
  static constexpr bool is_steady = true;

  static time_point now() noexcept {
    return time_point(duration(__ycxx::__detail::__pal_clock_ns(__ycxx_pal_clock_monotonic)));
  }
};

class high_resolution_clock {
public:
  using rep = steady_clock::rep;
  using period = steady_clock::period;
  using duration = steady_clock::duration;
  using time_point = steady_clock::time_point;
  static constexpr bool is_steady = true;

  static time_point now() noexcept { return steady_clock::now(); }
};

}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
class file_clock {
public:
  using rep = long long;
  using period = std::nano;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<file_clock>;
  static constexpr bool is_steady = false;

  static time_point now() noexcept {
    return time_point(duration(::__ycxx::__detail::__pal_clock_ns(__ycxx_pal_clock_realtime)));
  }
  // [time.clock.file.members]: the same epoch as system_clock.
  template <class _Duration>
  static std::chrono::sys_time<_Duration> to_sys(const std::chrono::file_time<_Duration>& t) {
    return std::chrono::sys_time<_Duration>(t.time_since_epoch());
  }
  template <class _Duration>
  static std::chrono::file_time<_Duration> from_sys(const std::chrono::sys_time<_Duration>& t) {
    return std::chrono::file_time<_Duration>(t.time_since_epoch());
  }
};
}} // namespace __ycxx::__adl_free
