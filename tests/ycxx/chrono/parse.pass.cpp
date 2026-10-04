// [time.parse]/2-: parse(fmt, tp) returns a manipulator; is >> parse(fmt, x) calls
// from_stream(is, fmt.c_str(), x [, abbrev, offset]). [time.cal.ymd.nonmembers] from_stream: if
// the parse fails to decode a valid year_month_day, failbit is set and ymd is not modified.
// [time.clock.system.nonmembers] from_stream for sys_time: "If %z is used and successfully
// parsed, the value is ... subtracted from the parsed time" (the result is UTC); %Z is stored
// in *abbrev. [time.duration.io]/3: from_stream for durations. [time.parse] Table 134: %Y %m %d
// %F %T %H %M %S (with fractional seconds for finer durations) %z %Z %b %a %j %%; white space
// in fmt matches zero or more white space characters.
#include <chrono>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

int main() {
  {
    std::istringstream is("2024-02-29");
    year_month_day ymd;
    is >> parse("%F", ymd);
    CHECK(is && ymd == 2024y / February / 29);
  }
  {
    std::istringstream is("2023-02-30");  // not a valid date
    year_month_day ymd = 2000y / January / 1;
    is >> parse("%F", ymd);
    CHECK(is.fail());
    CHECK(ymd == 2000y / January / 1);
  }
  {
    std::istringstream is("12/31/1999");
    year_month_day ymd;
    is >> parse("%m/%d/%Y", ymd);
    CHECK(is && ymd == 1999y / December / 31);
  }
  {
    std::istringstream is("2000-01-01 01:02:03");
    sys_seconds tp;
    is >> parse("%F %T", tp);
    CHECK(is && tp == sys_seconds{946'688'523s});
  }
  {
    std::istringstream is("2000-01-01 01:02:03.25");
    sys_time<milliseconds> tp;
    is >> parse("%F %T", tp);
    CHECK(is && tp.time_since_epoch() == 946'688'523'250ms);
  }
  {
    // %z: the offset is subtracted, giving UTC; %Z stores the abbreviation
    std::istringstream is("2000-01-01 03:02:03 +0200 EET");
    sys_seconds tp;
    std::string abbrev;
    minutes off{};
    is >> parse("%F %T %z %Z", tp, abbrev, off);
    CHECK(is);
    CHECK(tp == sys_seconds{946'688'523s});
    CHECK(abbrev == "EET" && off == 120min);
  }
  {
    std::istringstream is("01:30:15");
    seconds d;
    is >> parse("%H:%M:%S", d);
    CHECK(is && d == 5415s);
  }
  {
    std::istringstream is("Mar 7");
    month_day md;
    is >> parse("%b %d", md);
    CHECK(is && md == March / 7);
  }
  {
    std::istringstream is("2024  /  7");  // white space in fmt matches any amount
    year_month ym;
    is >> parse("%Y / %m", ym);
    CHECK(is && ym == 2024y / July);
  }
  {
    std::istringstream is("100%");
    year y;
    is >> parse("%Y%%", y);
    CHECK(is && y == year(100));
  }
  {
    // a literal mismatch fails
    std::istringstream is("2024-07");
    year_month ym = 1999y / January;
    is >> parse("%Y/%m", ym);
    CHECK(is.fail() && ym == 1999y / January);
  }
  {
    std::wistringstream is(L"2024-02-29");
    year_month_day ymd;
    is >> parse(L"%F", ymd);
    CHECK(is && ymd == 2024y / February / 29);
  }
  return 0;
}
