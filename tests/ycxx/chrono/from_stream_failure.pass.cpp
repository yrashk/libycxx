// from_stream failure modes. Each from_stream overload: "If the parse fails to decode a valid
// <type>, is.setstate(ios_base::failbit) is called and <out-parameter> is not modified"
// ([time.cal.day.nonmembers], [time.cal.month.nonmembers], [time.cal.year.nonmembers],
// [time.cal.wd.nonmembers], [time.cal.md.nonmembers], [time.cal.ym.nonmembers],
// [time.cal.ymd.nonmembers], [time.duration.io]/3, [time.clock.system.nonmembers]/6,
// [time.clock.local]/5). [time.parse]/16: a flag whose information the type cannot represent
// sets failbit (Example 1: "A duration cannot represent a weekday"); [time.parse]/17: failing to
// parse everything in the format string, or parsing too little to specify a complete value,
// sets failbit. [time.parse]/15: %n matches exactly one white space character; other
// characters of fmt are parsed unchanged; [time.parse] Table 134: %z is "[+|-]hh[mm]".
#include <chrono>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

// Parses in with fmt into a copy of init; true when failbit is set and the value is unchanged.
template <class T>
static bool fails(const char* in, const char* fmt, T init) {
  T v = init;
  std::istringstream is(in);
  is >> parse(fmt, v);
  return is.fail() && v == init;
}

int main() {
  // invalid values of the calendar types
  CHECK(fails("32", "%d", day(7)));
  CHECK(fails("00", "%d", day(7)));
  CHECK(fails("13", "%m", month(7)));
  CHECK(fails("0", "%m", month(7)));
  CHECK(fails("Foo", "%b", month(7)));
  CHECK(fails("Funday", "%a", weekday(3)));
  CHECK(fails("9", "%w", weekday(3)));
  CHECK(fails("abcd", "%Y", year(7)));
  CHECK(fails("02/30", "%m/%d", month_day(March / 3d)));
  CHECK(fails("04/31", "%m/%d", month_day(March / 3d)));
  CHECK(fails("2024-13", "%Y-%m", year_month(1999y / March)));
  CHECK(fails("2023-02-29", "%F", year_month_day(1999y / March / 3d)));
  CHECK(fails("2024-04-31", "%Y-%m-%d", year_month_day(1999y / March / 3d)));
  CHECK(fails("2023 366", "%Y %j", year_month_day(1999y / March / 3d)));
  CHECK(fails("2024-02-30 10:00:00", "%F %T", sys_seconds(5s)));
  CHECK(fails("2024-02-30 10:00:00", "%F %T", local_seconds(5s)));

  // incomplete information
  CHECK(fails("2024-07", "%Y-%m", year_month_day(1999y / March / 3d)));
  CHECK(fails("07-04", "%m-%d", year_month_day(1999y / March / 3d)));
  CHECK(fails("2024", "%Y", year_month(1999y / March)));
  CHECK(fails("7", "%d", month_day(March / 3d)));

  // a flag the type cannot represent
  CHECK(fails("Monday", "%a", seconds(42)));
  CHECK(fails("2024", "%Y", month(7)));
  CHECK(fails("Mar", "%b", day(7)));

  // input ends early, or does not match the format
  CHECK(fails("", "%F", year_month_day(1999y / March / 3d)));
  CHECK(fails("2024-0", "%F", year_month_day(1999y / March / 3d)));
  CHECK(fails("2024-07-04", "%Y/%m/%d", year_month_day(1999y / March / 3d)));
  CHECK(fails("12:", "%H:%M", minutes(42)));
  CHECK(fails("xx:30", "%H:%M", minutes(42)));
  CHECK(fails("202407", "%Y%n%m", year_month(1999y / March)));  // %n needs one white space
  CHECK(fails("2024-07-04", "%F extra", year_month_day(1999y / March / 3d)));

  // a bad %z: the time point is unchanged and failbit is set
  {
    sys_seconds tp(5s);
    minutes off(17min);
    std::istringstream is("2000-01-01 00:00:00 X0400");
    is >> parse("%F %T %z", tp, off);
    CHECK(is.fail() && tp == sys_seconds(5s));
  }

  // a stream already in a failed state extracts nothing ([time.parse]/15: from_stream behaves as an
  // unformatted input function, whose sentry fails)
  {
    year_month_day ymd = 1999y / March / 3d;
    std::istringstream is("2024-07-04");
    is.setstate(std::ios_base::failbit);
    is >> parse("%F", ymd);
    CHECK(is.fail() && ymd == 1999y / March / 3d);
  }

  // success after failure needs clear(); the stream position after a failure is not checked.
  {
    std::istringstream is("2024-07-04");
    year_month_day ymd = 1999y / March / 3d;
    is >> parse("%F", ymd);
    CHECK(!is.fail() && ymd == 2024y / July / 4d);
  }

  // wide
  {
    std::wistringstream is(L"2023-02-29");
    year_month_day ymd = 1999y / March / 3d;
    is >> parse(L"%F", ymd);
    CHECK(is.fail() && ymd == 1999y / March / 3d);
  }
  return 0;
}
