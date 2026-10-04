// [time.clock.system.nonmembers]/2-3: os << sys_time<Duration> (Duration{1} < days{1}) is
// format(os.getloc(), "{:L%F %T}", tp); Example 1: sys_seconds{0s} -> "1970-01-01 00:00:00",
// {946'688'523s} -> "2000-01-01 01:02:03". [time.format]/10: for sys_time, %Z is "UTC" and %z
// formats an offset of 0min ("+0000"; %Ez / %Oz "+00:00", Table 133). /15: local_time with %Z
// or %z throws format_error. /7: without chrono-specs the object is formatted as if streamed.
// [time.hms.nonmembers]/1-2: os << hh_mm_ss is "{:L%T}" (Example 1: -01:08:03.007,
// 01:08:03.007, 18:15:45.123, 18:15:45). Table 133 %I %p (the "C" locale: AM / PM), %r, %c,
// %x, %X in the "C" locale, %S with subsecond precision.
#include <chrono>
#include <format>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

template <class T>
static std::string str(const T& t) {
  std::ostringstream os;
  os << t;
  return os.str();
}

int main() {
  CHECK(str(sys_seconds{0s}) == "1970-01-01 00:00:00");
  CHECK(str(sys_seconds{946'684'800s}) == "2000-01-01 00:00:00");
  CHECK(str(sys_seconds{946'688'523s}) == "2000-01-01 01:02:03");
  CHECK(str(sys_time<milliseconds>{946'688'523'250ms}) == "2000-01-01 01:02:03.250");
  CHECK(str(sys_time<minutes>{minutes(61)}) == "1970-01-01 01:01:00");
  CHECK(str(sys_seconds{-1s}) == "1969-12-31 23:59:59");

  const sys_seconds t{946'688'523s};  // 2000-01-01 01:02:03, a Saturday
  CHECK(std::format("{}", t) == "2000-01-01 01:02:03");
  CHECK(std::format("{:%F %T %Z}", t) == "2000-01-01 01:02:03 UTC");
  CHECK(std::format("{:%z|%Ez|%Oz}", t) == "+0000|+00:00|+00:00");
  CHECK(std::format("{:%I %p}", t) == "01 AM");
  CHECK(std::format("{:%I %p}", t + 12h) == "01 PM");
  CHECK(std::format("{:%I %p}", sys_seconds{0s}) == "12 AM");
  CHECK(std::format("{:%r}", t + 12h) == "01:02:03 PM");
  CHECK(std::format("{:%x}", t) == "01/01/00");
  CHECK(std::format("{:%X}", t) == "01:02:03");
  CHECK(std::format("{:%c}", t) == "Sat Jan  1 01:02:03 2000");
  CHECK(std::format("{:%a %j %S}", t) == "Sat 001 03");
  CHECK(std::format("{:%T}", sys_time<microseconds>{1'500'000us}) == "00:00:01.500000");
  CHECK(std::format("{:%F}", sys_days{days(1)}) == "1970-01-02");

  // other clocks
  CHECK(std::format("{:%F %T %Z}", utc_seconds{0s} + (sys_days{1972y / January / 1} - sys_days{1970y / January / 1})) ==
        "1972-01-01 00:00:00 UTC");
  CHECK(std::format("{:%Z}", tai_seconds{0s}) == "TAI");
  CHECK(std::format("{:%F %T}", tai_seconds{0s}) == "1958-01-01 00:00:00");
  CHECK(std::format("{:%Z}", gps_seconds{0s}) == "GPS");
  CHECK(std::format("{:%F %T}", gps_seconds{0s}) == "1980-01-06 00:00:00");
  // a leap second is formatted with 60 seconds ([time.format]/11)
  utc_seconds leap = clock_cast<utc_clock>(sys_seconds{sys_days{2017y / January / 1}}) - 1s;
  CHECK(std::format("{:%F %T}", leap) == "2016-12-31 23:59:60");

  // local_time: no zone information
  local_seconds lt{3600s};
  CHECK(std::format("{:%F %T}", lt) == "1970-01-01 01:00:00");
  bool thrown = false;
  try {
    (void)std::vformat("{:%Z}", std::make_format_args(lt));
  } catch (const std::format_error&) {
    thrown = true;
  }
  CHECK(thrown);
  thrown = false;
  try {
    (void)std::vformat("{:%z}", std::make_format_args(lt));
  } catch (const std::format_error&) {
    thrown = true;
  }
  CHECK(thrown);

  // hh_mm_ss, Example 1
  CHECK(str(hh_mm_ss{-4083007ms}) == "-01:08:03.007");
  CHECK(str(hh_mm_ss{4083007ms}) == "01:08:03.007");
  CHECK(str(hh_mm_ss{65745123ms}) == "18:15:45.123");
  CHECK(str(hh_mm_ss{65745s}) == "18:15:45");
  CHECK(std::format("{:%H|%M|%S}", hh_mm_ss{3723s}) == "01|02|03");
  return 0;
}
