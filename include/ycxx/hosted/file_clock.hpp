// libycxx hosted: std::chrono::file_clock for <filesystem>'s file_time_type ([time.clock.file]).
//
// MERGE NOTE (temporary): <chrono> is being written concurrently (ycxx/core/chrono_base.hpp with
// duration and time_point, ycxx/hosted/chrono_clocks.hpp with the clocks, file_clock among
// them). Until it is merged, this file supplies the minimal subset that <filesystem> needs:
// duration and time_point with construction, count/time_since_epoch, min/max, comparison and
// same-type arithmetic, plus file_clock (nanoseconds in a long long, the epoch of system_clock,
// i.e. the Unix epoch; now() on the PAL's realtime clock). The names and the layout match the
// concurrent design (file_clock is ycxx::adl_free::file_clock, aliased as
// std::chrono::file_clock), so on merge the whole body below is replaced by
//   #include <ycxx/hosted/chrono_clocks.hpp>
// and nothing in <filesystem> or src/hosted/filesystem.cpp changes.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/ratio.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/pal.h>

namespace std::chrono {

template <class Rep, class Period = ratio<1>>
class duration {
  Rep r_ = Rep();

public:
  using rep = Rep;
  using period = typename Period::type;

  constexpr duration() = default;
  template <class Rep2>
    requires is_convertible_v<const Rep2&, Rep>
  constexpr explicit duration(const Rep2& r) : r_(static_cast<Rep>(r)) {}

  constexpr rep count() const { return r_; }
  static constexpr duration zero() noexcept { return duration(Rep(0)); }
  static constexpr duration min() noexcept { return duration(numeric_limits<Rep>::lowest()); }
  static constexpr duration max() noexcept { return duration(numeric_limits<Rep>::max()); }

  constexpr duration operator+() const { return *this; }
  constexpr duration operator-() const { return duration(-r_); }
  constexpr duration& operator+=(const duration& d) {
    r_ += d.r_;
    return *this;
  }
  constexpr duration& operator-=(const duration& d) {
    r_ -= d.r_;
    return *this;
  }
  friend constexpr duration operator+(duration a, const duration& b) { return a += b; }
  friend constexpr duration operator-(duration a, const duration& b) { return a -= b; }
  friend constexpr bool operator==(const duration& a, const duration& b) { return a.r_ == b.r_; }
  friend constexpr auto operator<=>(const duration& a, const duration& b) { return a.r_ <=> b.r_; }
};

using nanoseconds = duration<long long, nano>;

template <class Clock, class Duration = typename Clock::duration>
class time_point {
  Duration d_;

public:
  using clock = Clock;
  using duration = Duration;
  using rep = typename duration::rep;
  using period = typename duration::period;

  constexpr time_point() : d_(duration::zero()) {}
  constexpr explicit time_point(const duration& d) : d_(d) {}

  constexpr duration time_since_epoch() const { return d_; }
  static constexpr time_point min() noexcept { return time_point(duration::min()); }
  static constexpr time_point max() noexcept { return time_point(duration::max()); }

  constexpr time_point& operator+=(const duration& d) {
    d_ += d;
    return *this;
  }
  constexpr time_point& operator-=(const duration& d) {
    d_ -= d;
    return *this;
  }
  friend constexpr time_point operator+(time_point t, const duration& d) { return t += d; }
  friend constexpr time_point operator-(time_point t, const duration& d) { return t -= d; }
  friend constexpr duration operator-(const time_point& a, const time_point& b) { return a.d_ - b.d_; }
  friend constexpr bool operator==(const time_point& a, const time_point& b) { return a.d_ == b.d_; }
  friend constexpr auto operator<=>(const time_point& a, const time_point& b) { return a.d_ <=> b.d_; }
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
    ycxx_pal_i64 sec = 0, nsec = 0;
    ::ycxx_pal_clock_now(ycxx_pal_clock_realtime, &sec, &nsec);
    return time_point(duration(static_cast<long long>(sec) * 1'000'000'000 + static_cast<long long>(nsec)));
  }
};
} // namespace ycxx::adl_free

namespace std::chrono {
using file_clock = ycxx::adl_free::file_clock;
template <class Duration>
using file_time = time_point<file_clock, Duration>;
} // namespace std::chrono
