// [ctime.syn]: namespace std declares clock, difftime, mktime, timegm, time, timespec_get,
// timespec_getres, gmtime, gmtime_r, localtime, localtime_r and strftime, the types clock_t,
// time_t, timespec and tm, and the macros CLOCKS_PER_SEC and TIME_UTC; /1 the contents are
// those of ISO C 7.29 (C23). Footnote: strftime supports the C conversion specifiers C, D, e,
// F, g, G, h, r, R, t, T, u, V and z, and the modifiers E and O.
// ISO C 7.29.2.4 timegm: the inverse of gmtime (UTC calendar time to time_t), normalizing the
// fields; 7.29.2.6 timespec_get(ts, base): returns base on success; 7.29.2.7
// timespec_getres(ts, base): returns base and the resolution (positive) for a supported base;
// 7.29.3.4 gmtime_r and 7.29.3.5 localtime_r: as gmtime/localtime, into the caller's buffer,
// returning it.
// The process runs in UTC (POSIX TZ "UTC0", set before any conversion): ISO C's struct tm has no
// UTC offset, so what %z writes for a gmtime result is the platform's choice (glibc: the tm's own
// tm_gmtoff, +0000; Darwin: the local zone's offset), as the C standard leaves it ("the offset from
// UTC ... or by no characters if no time zone is determinable", 7.29.3.5); in UTC both agree.
#include <ctime>
#include <stdlib.h> // setenv (POSIX)
#include <time.h>   // tzset (POSIX)
#include <cstring>
#include <string>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::timegm(static_cast<std::tm*>(nullptr))), std::time_t>);
static_assert(std::is_same_v<decltype(std::gmtime_r(static_cast<const std::time_t*>(nullptr), static_cast<std::tm*>(nullptr))), std::tm*>);
static_assert(std::is_same_v<decltype(std::localtime_r(static_cast<const std::time_t*>(nullptr), static_cast<std::tm*>(nullptr))), std::tm*>);
static_assert(std::is_same_v<decltype(std::timespec_getres(static_cast<std::timespec*>(nullptr), 0)), int>);
static_assert(std::is_same_v<decltype(std::timespec_get(static_cast<std::timespec*>(nullptr), 0)), int>);
static_assert(std::is_same_v<decltype(std::clock()), std::clock_t>);
static_assert(std::is_same_v<decltype(std::difftime(std::time_t{}, std::time_t{})), double>);
static_assert(CLOCKS_PER_SEC > 0);
static_assert(TIME_UTC > 0);

static std::string ftime(const char* f, const std::tm& t) {
  char buf[128];
  std::size_t n = std::strftime(buf, sizeof buf, f, &t);
  return std::string(buf, n);
}

int main() {
  ::setenv("TZ", "UTC0", 1);
  ::tzset();
  std::tm t{};
  t.tm_year = 2026 - 1900;
  t.tm_mon = 0;
  t.tm_mday = 4;  // Sunday, ISO week 2026-W01
  t.tm_hour = 7;
  t.tm_min = 5;
  t.tm_sec = 9;
  std::time_t tt = std::timegm(&t);
  CHECK(tt == 1767510309);
  CHECK(t.tm_wday == 0 && t.tm_yday == 3);  // normalized fields filled in
  std::tm back{};
  CHECK(std::gmtime_r(&tt, &back) == &back);
  CHECK(back.tm_year == 126 && back.tm_mon == 0 && back.tm_mday == 4 && back.tm_hour == 7 && back.tm_sec == 9);
  CHECK(std::timegm(&back) == tt);
  // normalization: 2026-01-32 25:61:00 is 2026-02-02 02:01:00
  std::tm n{};
  n.tm_year = 126;
  n.tm_mday = 32;
  n.tm_hour = 25;
  n.tm_min = 61;
  std::time_t nt = std::timegm(&n);
  CHECK(n.tm_mon == 1 && n.tm_mday == 2 && n.tm_hour == 2 && n.tm_min == 1);
  CHECK(nt == 1769997660);
  std::tm loc{};
  CHECK(std::localtime_r(&tt, &loc) == &loc);
  CHECK(std::mktime(&loc) == tt);
  CHECK(std::difftime(tt + 90, tt) == 90.0);

  std::timespec ts{};
  CHECK(std::timespec_get(&ts, TIME_UTC) == TIME_UTC);
  CHECK(ts.tv_sec > 1700000000 && ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000);
  std::timespec res{};
  CHECK(std::timespec_getres(&res, TIME_UTC) == TIME_UTC);
  CHECK(res.tv_sec >= 0 && res.tv_nsec >= 0 && (res.tv_sec > 0 || res.tv_nsec > 0));
  CHECK(std::timespec_getres(nullptr, TIME_UTC) == TIME_UTC);
  CHECK(std::time(nullptr) >= ts.tv_sec);
  CHECK(std::clock() != static_cast<std::clock_t>(-1));

  // the C conversion specifiers named in [ctime.syn] (C locale)
  CHECK(ftime("%C|%D|%e|%F|%g|%G|%h|%r|%R|%t|%T|%u|%V|%z", back) ==
        "20|01/04/26| 4|2026-01-04|26|2026|Jan|07:05:09 AM|07:05|\t|07:05:09|7|01|+0000");
  CHECK(ftime("%Ey %EY %Od %Oe %OH %OI %Om %OM %OS %Ou %OU %OV %Ow %OW %Oy", back) ==
        "26 2026 04  4 07 07 01 05 09 7 01 01 0 00 26");
  CHECK(ftime("%Ec|%EC|%Ex|%EX", back) == ftime("%c|%C|%x|%X", back));
  std::tm sat{};
  sat.tm_year = 2021 - 1900;
  sat.tm_mday = 2;  // 2021-01-02 is in ISO week 2020-W53
  std::timegm(&sat);
  CHECK(ftime("%G-W%V-%u %g", sat) == "2020-W53-6 20");
}
