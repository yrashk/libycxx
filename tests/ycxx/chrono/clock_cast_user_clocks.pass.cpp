// [time.clock.cast.fn]/1-/3: clock_cast<DestClock>(t) uses the well-formed expression among
// (1.1) the direct clock_time_conversion<Dest, Source>, (1.2) through system_clock, (1.3) through
// utc_clock, (1.4) Source -> system -> utc -> Dest, (1.5) Source -> utc -> system -> Dest, that
// makes the fewest operator() calls; it is constrained on one of them being well-formed.
// [time.clock.cast.sys]/1-/6: clock_time_conversion<system_clock, C> calls C::to_sys,
// <C, system_clock> calls C::from_sys; [time.clock.cast.utc]: to_utc / from_utc likewise.
// [time.clock.cast.id]/1: clock_time_conversion<C, C> returns t.
// [time.clock.cast.sys.utc]/1-/2: utc_clock::from_sys / to_sys.
// A user specialization of clock_time_conversion ([time.clock.conv]) is a direct conversion.
#include <chrono>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

// system time shifted by one hour, convertible through system_clock
struct sysish_clock {
  using rep = long long;
  using period = std::ratio<1>;
  using duration = seconds;
  using time_point = std::chrono::time_point<sysish_clock>;
  static constexpr bool is_steady = false;
  static time_point now();
  template <class D>
  static sys_time<D> to_sys(const std::chrono::time_point<sysish_clock, D>& t) {
    return sys_time<D>(t.time_since_epoch() - 1h);
  }
  template <class D>
  static std::chrono::time_point<sysish_clock, D> from_sys(const sys_time<D>& t) {
    return std::chrono::time_point<sysish_clock, D>(t.time_since_epoch() + 1h);
  }
};

// utc time shifted by one minute, convertible through utc_clock only
struct utcish_clock {
  using rep = long long;
  using period = std::ratio<1>;
  using duration = seconds;
  using time_point = std::chrono::time_point<utcish_clock>;
  static constexpr bool is_steady = false;
  static time_point now();
  template <class D>
  static utc_time<D> to_utc(const std::chrono::time_point<utcish_clock, D>& t) {
    return utc_time<D>(t.time_since_epoch() + 1min);
  }
  template <class D>
  static std::chrono::time_point<utcish_clock, D> from_utc(const utc_time<D>& t) {
    return std::chrono::time_point<utcish_clock, D>(t.time_since_epoch() - 1min);
  }
};

// a clock with both, plus a direct conversion from sysish_clock that the two-step path must lose to
struct both_clock {
  using rep = long long;
  using period = std::ratio<1>;
  using duration = seconds;
  using time_point = std::chrono::time_point<both_clock>;
  static constexpr bool is_steady = false;
  static time_point now();
  template <class D>
  static sys_time<D> to_sys(const std::chrono::time_point<both_clock, D>& t) { return sys_time<D>(t.time_since_epoch()); }
  template <class D>
  static std::chrono::time_point<both_clock, D> from_sys(const sys_time<D>& t) {
    return std::chrono::time_point<both_clock, D>(t.time_since_epoch());
  }
};
template <>
struct std::chrono::clock_time_conversion<both_clock, sysish_clock> {
  template <class D>
  std::chrono::time_point<both_clock, D> operator()(const std::chrono::time_point<sysish_clock, D>&) const {
    return std::chrono::time_point<both_clock, D>(D(42));
  }
};

struct island_clock {  // no conversions at all
  using rep = long long;
  using period = std::ratio<1>;
  using duration = seconds;
  using time_point = std::chrono::time_point<island_clock>;
  static constexpr bool is_steady = false;
  static time_point now();
};

template <class Dest, class TP>
concept castable = requires(TP t) { clock_cast<Dest>(t); };

int main() {
  const sys_seconds s0 = sys_days{2000y / January / 1};  // after 1972: 22 leap seconds before it
  const time_point<sysish_clock, seconds> a(s0.time_since_epoch() + 1h);

  // (1.1) identity
  static_assert(std::is_same_v<decltype(clock_cast<sysish_clock>(a)), time_point<sysish_clock, seconds>>);
  CHECK(clock_cast<sysish_clock>(a) == a);
  // (1.2) via system_clock: 1 call to system_clock, and to utc_clock in 2 calls
  static_assert(std::is_same_v<decltype(clock_cast<system_clock>(a)), sys_seconds>);
  CHECK(clock_cast<system_clock>(a) == s0);
  auto u = clock_cast<utc_clock>(a);
  static_assert(std::is_same_v<decltype(u), utc_seconds>);
  CHECK(u == utc_clock::from_sys(s0));
  CHECK(u.time_since_epoch() == s0.time_since_epoch() + 22s);
  CHECK(clock_cast<sysish_clock>(s0) == a);

  // (1.3) via utc_clock
  const time_point<utcish_clock, seconds> b(u.time_since_epoch() - 1min);
  CHECK(clock_cast<utc_clock>(b) == u);
  CHECK(clock_cast<system_clock>(b) == s0);  // utc -> system: (1.3) with the utc/system conversion
  CHECK(clock_cast<utcish_clock>(s0) == b);
  // tai_clock converts with to_utc / from_utc
  auto t = clock_cast<tai_clock>(b);
  CHECK(t == tai_clock::from_utc(u));

  // (1.4) sysish -> system -> utc -> utcish, (1.5) utcish -> utc -> system -> sysish
  CHECK(clock_cast<utcish_clock>(a) == b);
  CHECK(clock_cast<sysish_clock>(b) == a);
  CHECK(clock_cast<tai_clock>(a) == t);
  CHECK(clock_cast<gps_clock>(a) == gps_clock::from_utc(u));

  // the direct user conversion wins over sysish -> system -> both
  CHECK(clock_cast<both_clock>(a).time_since_epoch() == 42s);
  // the reverse direction has no direct conversion: through system_clock
  const time_point<both_clock, seconds> c(s0.time_since_epoch());
  CHECK(clock_cast<sysish_clock>(c) == a);

  // finer durations are kept
  const time_point<sysish_clock, milliseconds> am = a + 5ms;
  static_assert(std::is_same_v<decltype(clock_cast<system_clock>(am)), sys_time<milliseconds>>);
  CHECK(clock_cast<system_clock>(am) == s0 + 5ms);
  static_assert(std::is_same_v<decltype(clock_cast<utc_clock>(am)), utc_time<milliseconds>>);

  // constraints
  static_assert(!castable<island_clock, decltype(a)>);
  static_assert(!castable<system_clock, time_point<island_clock, seconds>>);
  static_assert(castable<file_clock, decltype(a)>);
  return 0;
}
