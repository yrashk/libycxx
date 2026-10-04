// [time.parse] Table 134, flag by flag (the "C" locale, the global default):
// %z "[+|-]hh[mm]" ("-0430" is 4h30 behind UTC, "04" is 4h ahead); %Ez and %Oz "[+|-]h[h][:mm]"
// ("-04:30", "4"); %j the day of the year (January 1 is 1), or a number of days when parsing a
// duration; %U / %W the Sunday / Monday based week number (days before the first Sunday /
// Monday are in week 00); %G / %V the ISO 8601 week-based year and week; %u (1-7, Monday is 1)
// and %w (0-6, Sunday is 0); %y the last two digits, [69, 99] meaning 1969-1999 and [00, 68]
// 2000-2068 unless the century is given with %C (Example 2: "%3C %y" of "-20 76" is -1976);
// %D is %m/%d/%y; %e is %d; %a %A %b %B %h accept full or abbreviated case-insensitive names;
// %I with %p (12-hour clock), %R is %H:%M; %n matches exactly one white space character, %t
// zero or one; a width N bounds the characters read ("%NY", "%NH"); %Z parses one word of
// alphanumerics and '_', '/', '-', '+'. [time.parse]/16: "if a flag refers to a time of day
// ... a specialization of duration is parsed as the time of day elapsed since midnight".
// [time.clock.system.nonmembers]/6: for sys_time, %z "(or a modified variant)" is stored in
// *offset and subtracted from the parsed timestamp; [time.clock.local]/5: for local_time it is
// only stored. %x / %X are the locale's date and time representations: in the "C" locale
// ISO C 7.29.3.5 gives "%m/%d/%y" and "%H:%M:%S".
#include <chrono>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

template <class T, class... Extra>
static bool parses(const char* in, const char* fmt, T& out, Extra&... extra) {
  std::istringstream is(in);
  is >> parse(fmt, out, extra...);
  return !is.fail();
}

int main() {
  const sys_seconds midnight2000 = sys_days{2000y / January / 1};
  // %z and its modified forms: offsets stored and subtracted (sys_time)
  {
    sys_seconds tp;
    minutes off{};
    CHECK(parses("2000-01-01 00:00:00 -0430", "%F %T %z", tp, off));
    CHECK(off == -270min && tp == midnight2000 + 4h + 30min);
    CHECK(parses("2000-01-01 00:00:00 04", "%F %T %z", tp, off));
    CHECK(off == 240min && tp == midnight2000 - 4h);
    CHECK(parses("2000-01-01 00:00:00 +0545", "%F %T %z", tp, off));
    CHECK(off == 345min && tp == midnight2000 - 5h - 45min);
    CHECK(parses("2000-01-01 00:00:00 -04:30", "%F %T %Ez", tp, off));
    CHECK(off == -270min && tp == midnight2000 + 4h + 30min);
    CHECK(parses("2000-01-01 00:00:00 4 ", "%F %T %Ez", tp, off));
    CHECK(off == 240min && tp == midnight2000 - 4h);
    CHECK(parses("2000-01-01 00:00:00 +05:45", "%F %T %Oz", tp, off));
    CHECK(off == 345min && tp == midnight2000 - 5h - 45min);
    CHECK(parses("2000-01-01 00:00:00 -9 ", "%F %T %Oz", tp, off));
    CHECK(off == -540min && tp == midnight2000 + 9h);
    CHECK(parses("2000-01-01 00:00:00 +10:00", "%F %T %Ez", tp, off));
    CHECK(off == 600min);
    // without an offset argument, the offset still applies to a sys_time
    CHECK(parses("2000-01-01 02:00:00 +0200", "%F %T %z", tp));
    CHECK(tp == midnight2000);
  }
  {
    // local_time: the offset is stored, not applied
    local_seconds lt;
    minutes off{};
    CHECK(parses("2000-01-01 00:00:00 -0430", "%F %T %z", lt, off));
    CHECK(off == -270min && lt == local_days{2000y / January / 1});
  }
  {
    // %Z: one word; abbrev receives it
    sys_seconds tp;
    std::string abbrev;
    CHECK(parses("2000-01-01 00:00:00 America/New_York rest", "%F %T %Z", tp, abbrev));
    CHECK(abbrev == "America/New_York" && tp == midnight2000);
    CHECK(parses("2000-01-01 00:00:00 Etc/GMT+3", "%F %T %Z", tp, abbrev));
    CHECK(abbrev == "Etc/GMT+3");
    CHECK(parses("2000-01-01 00:00:00 my_zone-1", "%F %T %Z", tp, abbrev));
    CHECK(abbrev == "my_zone-1");
  }
  // %j
  {
    year_month_day ymd;
    CHECK(parses("2024 060", "%Y %j", ymd) && ymd == 2024y / February / 29);
    CHECK(parses("2023 365", "%Y %j", ymd) && ymd == 2023y / December / 31);
    CHECK(parses("2023 1 ", "%Y %j", ymd) && ymd == 2023y / January / 1);  // leading zeroes optional
    hours h;
    CHECK(parses("2 ", "%j", h) && h == 48h);  // a duration: a number of days
    days d;
    CHECK(parses("045", "%j", d) && d == days(45));
  }
  // %U and %W with %w / %u
  {
    year_month_day ymd;
    // 2024: the first Sunday is January 7 (start of %U week 01); the first Monday January 1.
    CHECK(parses("2024 09 2", "%Y %U %w", ymd) && ymd == 2024y / March / 5);
    CHECK(parses("2024 10 2", "%Y %W %u", ymd) && ymd == 2024y / March / 5);
    CHECK(parses("2024 00 1", "%Y %U %u", ymd) && ymd == 2024y / January / 1);
    // 2023-01-01 is a Sunday: %W week 00 holds only it.
    CHECK(parses("2023 00 7", "%Y %W %u", ymd) && ymd == 2023y / January / 1);
    CHECK(parses("2023 01 0", "%Y %U %w", ymd) && ymd == 2023y / January / 1);
  }
  // %G %V %u: ISO week dates
  {
    year_month_day ymd;
    CHECK(parses("2025-W01-1", "%G-W%V-%u", ymd) && ymd == 2024y / December / 30);
    CHECK(parses("2020-W53-5", "%G-W%V-%u", ymd) && ymd == 2021y / January / 1);
    CHECK(parses("2024-W10-2", "%G-W%V-%u", ymd) && ymd == 2024y / March / 5);
    CHECK(parses("2026 1 7", "%G %V %u", ymd) && ymd == 2026y / January / 4);
  }
  // %u and %w into a weekday
  {
    weekday wd;
    CHECK(parses("7", "%u", wd) && wd == Sunday);
    CHECK(parses("1", "%u", wd) && wd == Monday);
    CHECK(parses("0", "%w", wd) && wd == Sunday);
    CHECK(parses("6", "%w", wd) && wd == Saturday);
  }
  // %y, %C
  {
    year y;
    CHECK(parses("69", "%y", y) && y == 1969y);
    CHECK(parses("99", "%y", y) && y == 1999y);
    CHECK(parses("00", "%y", y) && y == 2000y);
    CHECK(parses("68", "%y", y) && y == 2068y);
    CHECK(parses("5 ", "%y", y) && y == 2005y);
    CHECK(parses("-20 76", "%3C %y", y) && y == year(-1976));  // [time.parse] Example 2
    CHECK(parses("19 05", "%C %y", y) && y == 1905y);
    CHECK(parses("2105", "%C%y", y) && y == 2105y);
    CHECK(parses("1234", "%2Y", y) && y == 12y);  // width bounds the characters read
    year_month_day ymd;
    CHECK(parses("03/05/24", "%D", ymd) && ymd == 2024y / March / 5);
    CHECK(parses("12/31/70", "%D", ymd) && ymd == 1970y / December / 31);
  }
  // names: full or abbreviated, any case
  {
    year_month_day ymd;
    CHECK(parses("fEbRuArY 29 2024", "%b %d %Y", ymd) && ymd == 2024y / February / 29);
    CHECK(parses("Feb 29 2024", "%B %d %Y", ymd) && ymd == 2024y / February / 29);
    CHECK(parses("MAR 7 2024", "%h %e %Y", ymd) && ymd == 2024y / March / 7);
    month m;
    CHECK(parses("september", "%B", m) && m == September);
    weekday wd;
    CHECK(parses("tUeSdAy", "%a", wd) && wd == Tuesday);
    CHECK(parses("SUN", "%A", wd) && wd == Sunday);
    CHECK(parses("Thursday", "%A", wd) && wd == Thursday);
    day dd;
    CHECK(parses("5 ", "%e", dd) && dd == 5d);
  }
  // time of day into durations
  {
    minutes m;
    CHECK(parses("01:30 PM", "%I:%M %p", m) && m == 13h + 30min);
    CHECK(parses("12:15 AM", "%I:%M %p", m) && m == 15min);
    CHECK(parses("12:15 PM", "%I:%M %p", m) && m == 12h + 15min);
    CHECK(parses("11:59 pm", "%I:%M %p", m) && m == 23h + 59min);
    CHECK(parses("13:45", "%R", m) && m == 13h + 45min);
    CHECK(parses("945", "%1H%M", m) && m == 9h + 45min);
    seconds s;
    CHECK(parses("7:5:3", "%H:%M:%S", s) && s == 7h + 5min + 3s);  // leading zeroes optional
    CHECK(parses("23:59:59", "%X", s) && s == 23h + 59min + 59s);
    year_month_day ymd;
    CHECK(parses("03/05/24", "%x", ymd) && ymd == 2024y / March / 5);
  }
  // %n and %t
  {
    year_month ym;
    CHECK(parses("2024\n07", "%Y%n%m", ym) && ym == 2024y / July);
    CHECK(parses("2024 07", "%Y%t%m", ym) && ym == 2024y / July);
    CHECK(parses("202407", "%Y%t%m", ym) && ym == 2024y / July);
    year y;
    CHECK(parses("2024\t  x", "%Y%n x", y) && y == 2024y);  // "%n " matches one or more
  }
  // A numeric field may be shorter than its maximum width N ("the maximum number of characters
  // to read", "Leading zeroes are permitted but not required"), also when the input ends there:
  // everything in fmt was parsed, so [time.parse]/17 does not apply.
  {
    sys_seconds tp;
    minutes off{};
    CHECK(parses("2000-01-01 00:00:00 4", "%F %T %Ez", tp, off) && off == 240min);
    year_month_day ymd;
    CHECK(parses("2023 1", "%Y %j", ymd) && ymd == 2023y / January / 1);
    hours h;
    CHECK(parses("2", "%j", h) && h == 48h);
    day dd;
    CHECK(parses("5", "%d", dd) && dd == 5d);
    minutes m;
    CHECK(parses("7:5", "%H:%M", m) && m == 7h + 5min);
  }
  // wide characters
  {
    std::wistringstream is(L"2025-W01-1 00:00:00 -04:30");
    sys_seconds tp;
    minutes off{};
    is >> parse(L"%G-W%V-%u %T %Ez", tp, off);
    CHECK(!is.fail() && off == -270min && tp == sys_days{2024y / December / 30} + 4h + 30min);
  }
  return 0;
}
