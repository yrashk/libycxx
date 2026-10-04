// libycxx hosted: the clocks of <chrono> ([time.clock.system], [time.clock.steady],
// [time.clock.hires], [time.clock.file]), on the PAL's ycxx_pal_clock_now.
//
// All three count nanoseconds in a long long. system_clock is the realtime clock (Unix time),
// steady_clock the monotonic clock, high_resolution_clock a distinct steady clock whose
// time_point is steady_clock's. file_clock has system_clock's epoch.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/pal.h>
#include <time.h>

namespace ycxx::detail {
// The current value of a PAL clock in nanoseconds.
inline long long pal_clock_ns(int clock) noexcept {
  ycxx_pal_i64 sec = 0, nsec = 0;
  ::ycxx_pal_clock_now(clock, &sec, &nsec);
  return static_cast<long long>(sec) * 1'000'000'000 + static_cast<long long>(nsec);
}
} // namespace ycxx::detail

namespace std::chrono {

class system_clock {
public:
  using rep = long long;
  using period = nano;
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<system_clock>;
  static constexpr bool is_steady = false;

  static time_point now() noexcept {
    return time_point(duration(ycxx::detail::pal_clock_ns(ycxx_pal_clock_realtime)));
  }
  // Truncated toward negative infinity to whole seconds.
  static ::time_t to_time_t(const time_point& t) noexcept {
    return static_cast<::time_t>(chrono::floor<seconds>(t.time_since_epoch()).count());
  }
  static time_point from_time_t(::time_t t) noexcept {
    return time_point(chrono::duration_cast<duration>(seconds(static_cast<seconds::rep>(t))));
  }
};

class steady_clock {
public:
  using rep = long long;
  using period = nano;
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<steady_clock, duration>;
  static constexpr bool is_steady = true;

  static time_point now() noexcept {
    return time_point(duration(ycxx::detail::pal_clock_ns(ycxx_pal_clock_monotonic)));
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

} // namespace std::chrono

namespace ycxx::adl_free {
class file_clock {
public:
  using rep = long long;
  using period = std::nano;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<file_clock>;
  static constexpr bool is_steady = false;

  static time_point now() noexcept {
    return time_point(duration(::ycxx::detail::pal_clock_ns(ycxx_pal_clock_realtime)));
  }
  // [time.clock.file.members]: the same epoch as system_clock.
  template <class Duration>
  static std::chrono::sys_time<Duration> to_sys(const std::chrono::file_time<Duration>& t) {
    return std::chrono::sys_time<Duration>(t.time_since_epoch());
  }
  template <class Duration>
  static std::chrono::file_time<Duration> from_sys(const std::chrono::sys_time<Duration>& t) {
    return std::chrono::file_time<Duration>(t.time_since_epoch());
  }
};
} // namespace ycxx::adl_free
