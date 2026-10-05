// [support.c.headers.other]/1: <time.h> places in the global namespace each name <ctime> places
// in std ([ctime.syn]: clock_t, time_t, timespec, tm, clock, difftime, mktime, timegm, time,
// timespec_get, timespec_getres, gmtime, gmtime_r, localtime, localtime_r, strftime), with the C23
// meaning (ISO C 7.29.2.4 timegm: UTC broken-down time to time_t; 7.29.2.7 timespec_getres:
// returns base and a positive resolution; 7.29.3.4 gmtime_r: into the caller's buffer, returning
// it). Only <time.h> is included.
#include <time.h>

#include "check.hpp"

template <class A, class B>
constexpr bool same = __is_same(A, B);

using ::clock;
using ::clock_t;
using ::difftime;
using ::gmtime;
using ::localtime;
using ::mktime;
using ::size_t;
using ::strftime;
using ::time;
using ::time_t;
using ::timespec;
using ::tm;

static_assert(same<decltype(::timegm(nullptr)), ::time_t>);
static_assert(same<decltype(::timespec_get(nullptr, 0)), int>);
static_assert(same<decltype(::timespec_getres(nullptr, 0)), int>);
static_assert(same<decltype(::gmtime_r(nullptr, nullptr)), ::tm*>);
static_assert(same<decltype(::localtime_r(nullptr, nullptr)), ::tm*>);
static_assert(CLOCKS_PER_SEC > 0 && TIME_UTC > 0);

int main() {
  ::tm t{};
  t.tm_year = 100;  // 2000-01-01T00:00:00Z
  t.tm_mday = 1;
  ::time_t v = ::timegm(&t);
  CHECK(v == 946684800);
  ::tm back{};
  CHECK(::gmtime_r(&v, &back) == &back && back.tm_year == 100 && back.tm_yday == 0 && back.tm_wday == 6);
  ::tm local{};
  CHECK(::localtime_r(&v, &local) == &local);
  ::timespec ts{};
  CHECK(::timespec_get(&ts, TIME_UTC) == TIME_UTC && ts.tv_sec > 946684800);
  ::timespec res{};
  CHECK(::timespec_getres(&res, TIME_UTC) == TIME_UTC && (res.tv_sec > 0 || res.tv_nsec > 0));
  return 0;
}
