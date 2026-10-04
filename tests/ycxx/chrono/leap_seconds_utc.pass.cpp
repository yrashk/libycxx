// Leap seconds and the utc / tai / gps clocks.
// [time.zone.leap.overview] Example 1: the database's leap seconds up to 2018-03-17 are the 27
// positive ones listed (date() is the first second after the insertion, e.g. 1972-07-01
// 00:00:00); [time.zone.leap.members]/2: value() is +1s or -1s.
// [time.clock.utc.overview] Example 1: clock_cast<utc_clock>(sys 2000-01-01) is 946'684'822s.
// [time.clock.utc.members]/2: to_sys of a time during a positive leap second returns "the last
// representable value of sys_time prior to the insertion"; /3 and its Example: from_sys counts
// the leap seconds inserted between 1970-01-01 and t, including one inserted exactly at t.
// [time.clock.utc.nonmembers]/2 Example: streaming utc_time across the 2015 leap second; /6:
// get_leap_second_info. [time.clock.tai.overview]/1: TAI is 10s ahead of UTC on 1958-01-01
// and 32s ahead on 2000-01-01; [time.clock.tai.nonmembers]/2 Example; [time.clock.gps.overview]
// /1: GPS counts from 1980-01-06 00:00:00 UTC and is 19s behind TAI. [time.format]/10-13: %Z is
// "UTC", "TAI" or "GPS", %z an offset of 0; a leap second formats with 60 seconds; TAI and GPS
// dates are those of a sys_time with the epoch shifted. [time.clock.cast.fn]: clock_cast.
#include <chrono>
#include <array>
#include <format>
#include <sstream>
#include <string>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

constexpr std::array<sys_days, 27> listed = {
    sys_days{1972y / July / 1},    sys_days{1973y / January / 1}, sys_days{1974y / January / 1},
    sys_days{1975y / January / 1}, sys_days{1976y / January / 1}, sys_days{1977y / January / 1},
    sys_days{1978y / January / 1}, sys_days{1979y / January / 1}, sys_days{1980y / January / 1},
    sys_days{1981y / July / 1},    sys_days{1982y / July / 1},    sys_days{1983y / July / 1},
    sys_days{1985y / July / 1},    sys_days{1988y / January / 1}, sys_days{1990y / January / 1},
    sys_days{1991y / January / 1}, sys_days{1992y / July / 1},    sys_days{1993y / July / 1},
    sys_days{1994y / July / 1},    sys_days{1996y / January / 1}, sys_days{1997y / July / 1},
    sys_days{1999y / January / 1}, sys_days{2006y / January / 1}, sys_days{2009y / January / 1},
    sys_days{2012y / July / 1},    sys_days{2015y / July / 1},    sys_days{2017y / January / 1}};

static_assert(std::is_same_v<decltype(utc_clock::to_sys(utc_time<milliseconds>{})), sys_time<milliseconds>>);
static_assert(std::is_same_v<decltype(utc_clock::from_sys(sys_days{})), utc_seconds>);
static_assert(std::is_same_v<decltype(get_leap_second_info(utc_seconds{}).elapsed), seconds>);
static_assert(std::is_same_v<decltype(std::declval<const leap_second&>().date()), sys_seconds>);
static_assert(std::is_same_v<decltype(clock_cast<tai_clock>(sys_days{})), tai_seconds>);
static_assert(std::is_same_v<decltype(clock_cast<gps_clock>(utc_time<nanoseconds>{})), gps_time<nanoseconds>>);
static_assert(std::is_signed_v<utc_clock::rep>);

template <class T>
std::string str(const T& t) {
  std::ostringstream os;
  os << t;
  return os.str();
}

int main() {
  // The database: the listed insertions first, all positive; any later ones come after 2017.
  const auto& ls = get_tzdb().leap_seconds;
  CHECK(ls.size() >= listed.size());
  for (std::size_t i = 0; i < listed.size(); ++i) CHECK(ls[i].date() == listed[i] && ls[i].value() == 1s);
  for (std::size_t i = listed.size(); i < ls.size(); ++i)
    CHECK(ls[i] > listed.back() && (ls[i].value() == 1s || ls[i].value() == -1s));

  // from_sys / to_sys.
  CHECK(clock_cast<utc_clock>(sys_seconds{sys_days{1970y / January / 1}}).time_since_epoch() == 0s);
  CHECK(clock_cast<utc_clock>(sys_seconds{sys_days{2000y / January / 1}}).time_since_epoch() == 946'684'822s);
  CHECK(utc_clock::from_sys(sys_days{1972y / July / 1} - 1s).time_since_epoch() ==
        (sys_days{1972y / July / 1} - 1s).time_since_epoch());
  CHECK(utc_clock::from_sys(sys_days{1972y / July / 1}).time_since_epoch() ==
        sys_days{1972y / July / 1}.time_since_epoch() + 1s);  // counted at its date
  {
    auto t = sys_days{July / 1 / 2015} - 2ns;  // the example of /3
    auto u = utc_clock::from_sys(t);
    CHECK(u.time_since_epoch() - t.time_since_epoch() == 25s);
    t += 1ns;
    u = utc_clock::from_sys(t);
    CHECK(u.time_since_epoch() - t.time_since_epoch() == 25s);
    t += 1ns;
    u = utc_clock::from_sys(t);
    CHECK(u.time_since_epoch() - t.time_since_epoch() == 26s);
    t += 1ns;
    u = utc_clock::from_sys(t);
    CHECK(u.time_since_epoch() - t.time_since_epoch() == 26s);
  }
  // During the leap second 2016-12-31 23:59:60: the last value before the insertion.
  const utc_seconds leap = utc_clock::from_sys(sys_days{2017y / January / 1}) - 1s;
  CHECK(utc_clock::to_sys(leap) == sys_days{2017y / January / 1} - 1s);
  CHECK(utc_clock::to_sys(utc_time<milliseconds>{leap + 500ms}) == sys_days{2017y / January / 1} - 1ms);
  CHECK(utc_clock::to_sys(utc_time<nanoseconds>{leap + 999'999'999ns}) == sys_days{2017y / January / 1} - 1ns);
  CHECK(utc_clock::to_sys(leap - 1s) == sys_days{2017y / January / 1} - 1s);  // 23:59:59 itself
  CHECK(utc_clock::to_sys(leap + 1s) == sys_days{2017y / January / 1});
  CHECK(utc_clock::from_sys(utc_clock::to_sys(leap + 1s)) == leap + 1s);
  CHECK(clock_cast<system_clock>(leap) == utc_clock::to_sys(leap));

  // get_leap_second_info.
  leap_second_info lsi = get_leap_second_info(leap);
  CHECK(lsi.is_leap_second && lsi.elapsed == 27s);
  lsi = get_leap_second_info(leap - 1s);
  CHECK(!lsi.is_leap_second && lsi.elapsed == 26s);
  lsi = get_leap_second_info(leap + 1s);
  CHECK(!lsi.is_leap_second && lsi.elapsed == 27s);
  lsi = get_leap_second_info(utc_time<milliseconds>{leap + 999ms});
  CHECK(lsi.is_leap_second && lsi.elapsed == 27s);
  lsi = get_leap_second_info(utc_seconds{});
  CHECK(!lsi.is_leap_second && lsi.elapsed == 0s);
  CHECK(get_leap_second_info(utc_clock::now()).elapsed >= 27s);
  CHECK(utc_clock::now() - clock_cast<utc_clock>(system_clock::now()) < 1min);

  // Streaming (the example of [time.clock.utc.nonmembers]/2) and formatting.
  {
    auto t = sys_days{July / 1 / 2015} - 500ms;
    auto u = clock_cast<utc_clock>(t);
    std::string out;
    for (auto i = 0; i < 8; ++i, u += 250ms) out += str(u) + " UTC\n";
    CHECK(out ==
          "2015-06-30 23:59:59.500 UTC\n"
          "2015-06-30 23:59:59.750 UTC\n"
          "2015-06-30 23:59:60.000 UTC\n"
          "2015-06-30 23:59:60.250 UTC\n"
          "2015-06-30 23:59:60.500 UTC\n"
          "2015-06-30 23:59:60.750 UTC\n"
          "2015-07-01 00:00:00.000 UTC\n"
          "2015-07-01 00:00:00.250 UTC\n");
  }
  CHECK(std::format("{:%T %Z %z}", leap) == "23:59:60 UTC +0000");
  CHECK(std::format("{:%S}", utc_time<milliseconds>{leap + 250ms}) == "60.250");
  CHECK(std::format("{:%F %T}", leap + 1s) == "2017-01-01 00:00:00");

  // TAI: 10s ahead in 1958, 32s ahead in 2000, 37s ahead after 2017.
  CHECK(clock_cast<tai_clock>(sys_seconds{sys_days{1958y / January / 1}} - 10s).time_since_epoch() == 0s);
  const sys_seconds y2000 = sys_days{2000y / January / 1};
  const tai_seconds tt = clock_cast<tai_clock>(y2000);
  CHECK(std::format("{0:%F %T %Z} == {1:%F %T %Z}", y2000, tt) == "2000-01-01 00:00:00 UTC == 2000-01-01 00:00:32 TAI");
  CHECK(str(tt) == "2000-01-01 00:00:32");
  CHECK(std::format("{:%T}", clock_cast<tai_clock>(sys_seconds{sys_days{2020y / January / 1}})) == "00:00:37");
  CHECK(clock_cast<tai_clock>(leap) - clock_cast<tai_clock>(leap - 1s) == 1s);  // TAI has no gap
  CHECK(clock_cast<utc_clock>(clock_cast<tai_clock>(leap)) == leap);
  CHECK(std::format("{:%z}", tt) == "+0000");

  // GPS: zero at the first Sunday of January 1980; 19s behind TAI; 18s ahead of UTC after 2017.
  CHECK(clock_cast<gps_clock>(sys_seconds{sys_days{1980y / January / Sunday[1]}}).time_since_epoch() == 0s);
  CHECK(sys_days{1980y / January / Sunday[1]} == sys_days{1980y / January / 6});
  const gps_seconds g = clock_cast<gps_clock>(sys_seconds{sys_days{2020y / January / 1}});
  CHECK(std::format("{:%F %T %Z}", g) == "2020-01-01 00:00:18 GPS");
  CHECK(str(g) == "2020-01-01 00:00:18");
  CHECK(clock_cast<tai_clock>(g) - tai_seconds{g.time_since_epoch()} ==
        sys_days{1980y / January / 6} - sys_days{1958y / January / 1} + 19s);
  CHECK(clock_cast<gps_clock>(clock_cast<tai_clock>(g)) == g);
  CHECK(clock_cast<system_clock>(g) == sys_days{2020y / January / 1});
  // Wide formatting of the leap second.
  CHECK(std::format(L"{:%T %Z}", leap) == L"23:59:60 UTC");
  return 0;
}
