// [time.clock.system.overview]/1: sys_time measures time since 1970-01-01 00:00:00 UTC excluding
// leap seconds: "sys_seconds{sys_days{1970y/January/1}}.time_since_epoch() is 0s." and
// "sys_seconds{sys_days{2000y/January/1}}.time_since_epoch() is 946'684'800s, which is
// 10'957 * 86'400s." [time.cal.wd.members]/2: 1970-01-01 is a Thursday.
#include <chrono>
#include "check.hpp"

using namespace std::chrono;

static_assert(sys_seconds{sys_days{1970y / January / 1}}.time_since_epoch() == 0s);
static_assert(sys_seconds{sys_days{2000y / January / 1}}.time_since_epoch() == 946'684'800s);
static_assert(sys_days{2000y / January / 1}.time_since_epoch() == days(10957));
static_assert(sys_days{1969y / December / 31}.time_since_epoch() == days(-1));
static_assert(sys_days{1600y / March / 1}.time_since_epoch() == days(-135080));
static_assert(sys_days{2038y / January / 19}.time_since_epoch() == days(24855));
static_assert(weekday{sys_days{1970y / January / 1}} == Thursday);
static_assert(weekday{sys_days{2000y / January / 1}} == Saturday);
static_assert(local_days{2000y / January / 1}.time_since_epoch() == days(10957));
static_assert(year_month_day{sys_days{days(0)}} == 1970y / January / 1);
static_assert(year_month_day{sys_days{days(-1)}} == 1969y / December / 31);
// The documented range limits of [time.cal.ymd.members]/14.
static_assert(year_month_day{sys_days{days(-12687428)}} == year(-32767) / January / 1);
static_assert(year_month_day{sys_days{days(11248737)}} == year(32767) / December / 31);
static_assert(sys_days{year_month_day{sys_days{days(-12687428)}}}.time_since_epoch() == days(-12687428));

int main() {
  CHECK(sys_seconds{sys_days{2000y / January / 1}}.time_since_epoch() == 10'957 * 86'400s);
}
