// clock_cast ([time.clock.cast.fn]/1-3): the best of the conversion expressions (1.1)-(1.5),
// the one with the fewest clock_time_conversion calls; identity conversions
// ([time.clock.cast.id]/1-3); sys <-> utc ([time.clock.cast.sys.utc]/1-2); through to_sys /
// from_sys ([time.clock.cast.sys]/1-6) and to_utc / from_utc ([time.clock.cast.utc]/1-6) of
// the source and destination clocks; a program-defined clock_time_conversion specialization
// (1.1) is used directly. [time.clock.file.members]/1: file_clock provides precisely one of
// the pairs to_sys / from_sys and to_utc / from_utc, "consistent with those specified by
// utc_clock, tai_clock, and gps_clock"; [time.clock.file.overview]/1: signed rep, noexcept
// now(); [time.format]/14: a file_time formats as the corresponding sys_time (%Z is "UTC").
// [filesystems]: file_time_type is file_time<...> ([fs.filesystem.syn]).
#include <chrono>
#include <filesystem>
#include <format>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

template <class C>
concept has_sys = requires(file_time<seconds> f, sys_seconds s) {
  C::to_sys(f);
  C::from_sys(s);
};
template <class C>
concept has_utc = requires(file_time<seconds> f, utc_seconds u) {
  C::to_utc(f);
  C::from_utc(u);
};
static_assert(has_sys<file_clock> != has_utc<file_clock>);  // precisely one of the two sets
static_assert(std::is_same_v<std::filesystem::file_time_type::clock, file_clock>);
static_assert(std::is_signed_v<file_clock::rep> && noexcept(file_clock::now()));

// Program-defined clocks: one with to_sys / from_sys (epoch 2000-01-01 UTC as sys time), one
// with to_utc / from_utc (epoch 2000-01-01 UTC as utc time), one with neither but a direct
// clock_time_conversion to system_clock.
struct SysClock {
  using rep = long long;
  using period = std::ratio<1>;
  using duration = seconds;
  using time_point = std::chrono::time_point<SysClock>;
  static constexpr bool is_steady = false;
  static time_point now() noexcept { return time_point{}; }
  static constexpr seconds shift = sys_days{2000y / January / 1}.time_since_epoch();
  template <class D>
  static sys_time<std::common_type_t<D, seconds>> to_sys(const std::chrono::time_point<SysClock, D>& t) {
    return sys_time<std::common_type_t<D, seconds>>{t.time_since_epoch() + shift};
  }
  template <class D>
  static std::chrono::time_point<SysClock, std::common_type_t<D, seconds>> from_sys(const sys_time<D>& t) {
    return std::chrono::time_point<SysClock, std::common_type_t<D, seconds>>{t.time_since_epoch() - shift};
  }
};
struct UtcClock {
  using rep = long long;
  using period = std::ratio<1>;
  using duration = seconds;
  using time_point = std::chrono::time_point<UtcClock>;
  static constexpr bool is_steady = false;
  static time_point now() noexcept { return time_point{}; }
  static constexpr seconds shift = sys_days{2000y / January / 1}.time_since_epoch() + 22s;
  template <class D>
  static utc_time<std::common_type_t<D, seconds>> to_utc(const std::chrono::time_point<UtcClock, D>& t) {
    return utc_time<std::common_type_t<D, seconds>>{t.time_since_epoch() + shift};
  }
  template <class D>
  static std::chrono::time_point<UtcClock, std::common_type_t<D, seconds>> from_utc(const utc_time<D>& t) {
    return std::chrono::time_point<UtcClock, std::common_type_t<D, seconds>>{t.time_since_epoch() - shift};
  }
};
struct DirectClock {
  using rep = long long;
  using period = std::ratio<1>;
  using duration = seconds;
  using time_point = std::chrono::time_point<DirectClock>;
  static constexpr bool is_steady = false;
  static time_point now() noexcept { return time_point{}; }
};
inline int direct_calls = 0;
template <>
struct std::chrono::clock_time_conversion<std::chrono::system_clock, DirectClock> {
  template <class D>
  sys_time<D> operator()(const std::chrono::time_point<DirectClock, D>& t) const {
    ++direct_calls;
    return sys_time<D>{t.time_since_epoch() + 1h};
  }
};
// A direct conversion that bypasses the general rules: SysClock -> UtcClock in one call.
inline int sys_to_utc_calls = 0;
template <>
struct std::chrono::clock_time_conversion<UtcClock, SysClock> {
  template <class D>
  std::chrono::time_point<UtcClock, D> operator()(const std::chrono::time_point<SysClock, D>& t) const {
    ++sys_to_utc_calls;
    return std::chrono::time_point<UtcClock, D>{t.time_since_epoch() + 1000h};  // deliberately "wrong"
  }
};

template <class Dest, class T>
concept castable = requires(T t) { clock_cast<Dest>(t); };
static_assert(castable<system_clock, file_time<seconds>> && castable<file_clock, sys_seconds>);
static_assert(castable<tai_clock, file_time<seconds>> && castable<file_clock, gps_seconds>);
static_assert(castable<utc_clock, time_point<SysClock>> && castable<gps_clock, time_point<SysClock>>);
static_assert(castable<system_clock, time_point<UtcClock>> && castable<file_clock, time_point<UtcClock>>);
static_assert(castable<system_clock, time_point<DirectClock>>);
static_assert(castable<utc_clock, time_point<DirectClock>>);    // (1.2) through system_clock
static_assert(!castable<DirectClock, sys_seconds> && !castable<DirectClock, utc_seconds>);  // no way back
static_assert(!castable<steady_clock, sys_seconds>);
static_assert(std::is_same_v<decltype(clock_cast<steady_clock>(steady_clock::now())), steady_clock::time_point>);

int main() {
  // Identity.
  const auto st = steady_clock::now();
  CHECK(clock_cast<steady_clock>(st) == st);
  const sys_time<milliseconds> sms{sys_days{2020y / March / 1} + 5ms};
  CHECK(clock_cast<system_clock>(sms) == sms);
  CHECK(clock_cast<utc_clock>(utc_seconds{42s}) == utc_seconds{42s});

  // file_clock: round trips and consistency with sys / utc / tai / gps.
  const sys_seconds s0 = sys_days{2017y / January / 1} - 1s;  // just before a leap second
  const auto f0 = clock_cast<file_clock>(s0);
  CHECK(clock_cast<system_clock>(f0) == s0);
  CHECK(clock_cast<utc_clock>(f0) == clock_cast<utc_clock>(s0));
  CHECK(clock_cast<tai_clock>(f0) == clock_cast<tai_clock>(s0));
  CHECK(clock_cast<gps_clock>(f0) == clock_cast<gps_clock>(s0));
  CHECK(clock_cast<file_clock>(clock_cast<tai_clock>(f0)) == f0);
  const auto f1 = clock_cast<file_clock>(sys_days{2017y / January / 1});
  CHECK(clock_cast<utc_clock>(f1) - clock_cast<utc_clock>(f0) == 2s);  // the leap second counts in UTC
  CHECK(clock_cast<system_clock>(f1) - clock_cast<system_clock>(f0) == 1s);
  const auto fms = clock_cast<file_clock>(sms);
  CHECK(clock_cast<system_clock>(fms) == sms);
  CHECK(std::format("{:%F %T %Z %z}", fms) == "2020-03-01 00:00:00.005 UTC +0000");
  CHECK(std::format("{:%F %T}", f0) == "2016-12-31 23:59:59");
  const auto fnow = file_clock::now();
  const auto snow = system_clock::now();
  CHECK(clock_cast<system_clock>(fnow) - snow < 1min && snow - clock_cast<system_clock>(fnow) < 1min);
  CHECK(std::filesystem::file_time_type::clock::now() >= fnow);

  // Program-defined clocks.
  const time_point<SysClock> sc{0s};
  CHECK(clock_cast<system_clock>(sc) == sys_days{2000y / January / 1});                    // (1.1)
  CHECK(clock_cast<utc_clock>(sc) == clock_cast<utc_clock>(sys_seconds{sys_days{2000y / January / 1}}));  // (1.2)
  CHECK(clock_cast<tai_clock>(sc) == clock_cast<tai_clock>(sys_seconds{sys_days{2000y / January / 1}}));  // (1.4)
  CHECK(clock_cast<SysClock>(sys_days{2000y / January / 2}) == time_point<SysClock>{24h});  // (1.1)
  CHECK(clock_cast<SysClock>(utc_clock::from_sys(sys_days{2000y / January / 2})) == time_point<SysClock>{24h});  // (1.2)
  const time_point<UtcClock> uc{0s};
  CHECK(clock_cast<utc_clock>(uc).time_since_epoch() == 946'684'822s);                    // (1.1)
  CHECK(clock_cast<system_clock>(uc) == sys_days{2000y / January / 1});                   // (1.3)
  CHECK(clock_cast<UtcClock>(sys_days{2000y / January / 1}) == uc);                        // (1.3)
  CHECK(clock_cast<UtcClock>(clock_cast<file_clock>(sys_days{2000y / January / 1})) == uc);
  CHECK(clock_cast<file_clock>(uc) == clock_cast<file_clock>(sys_days{2000y / January / 1}));
  CHECK(clock_cast<SysClock>(uc) == sc);  // (1.5), the only route
  // DirectClock: only the specialization to system_clock exists.
  CHECK(clock_cast<system_clock>(time_point<DirectClock>{0s}) == sys_seconds{1h} && direct_calls == 1);
  CHECK(clock_cast<SysClock>(time_point<DirectClock>{0s}) ==
        time_point<SysClock>{sys_seconds{1h}.time_since_epoch() - SysClock::shift});  // (1.2)
  CHECK(direct_calls == 2);
  CHECK(clock_cast<utc_clock>(time_point<DirectClock>{0s}) == utc_seconds{1h} && direct_calls == 3);  // (1.2)
  // The program-defined SysClock -> UtcClock specialization is the single-call (1.1) route.
  CHECK(clock_cast<UtcClock>(sc) == time_point<UtcClock>{1000h} && sys_to_utc_calls == 1);
  return 0;
}
