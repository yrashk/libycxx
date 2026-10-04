// Calendar edge cases.
// [time.cal.ymwd.members]/20: operator sys_days, when y_, m_ and the weekday are ok, is
// (index() - 1) * 7 days after the first weekday() of year()/month() (so index 5 of a month with
// only four such weekdays, and index 6, name days of the next month), 7 days before it for
// index 0; ok() additionally needs the index to be in [1, 5] and the date to fall in the month.
// [time.cal.ymd.members]: "For any value ymd of type year_month_day for which ymd.ok() is true,
// ymd == year_month_day{sys_days{ymd}} is true", including year::min()/January/1 and
// year::max()/December/31 ([time.cal.year.members]: min() is -32767, max() 32767);
// ymd + months is (ymd.year()/ymd.month() + dm)/ymd.day(), so the day is kept even when the
// result is !ok(), and the !ok() date converts with sys_days{y/m/1d} + (d - 1d).
// [time.cal.ymdlast.members]: ymdl + months is (year()/month() + dm)/last.
#include <chrono>
#include "check.hpp"

using namespace std::chrono;

constexpr bool test() {
  // February 2016 has four Sundays: 7, 14, 21, 28.
  year_month_weekday w5 = 2016y / February / Sunday[5];
  if (w5.ok() || sys_days{w5} != sys_days{2016y / March / 6}) return false;
  year_month_weekday w6 = 2016y / February / Sunday[6];
  if (w6.ok() || sys_days{w6} != sys_days{2016y / March / 13}) return false;
  if (year_month_weekday{sys_days{w5}} != 2016y / March / Sunday[1]) return false;
  // index 0 on a month whose first day is that weekday: a whole week before.
  if (sys_days{2016y / May / Sunday[0]} != sys_days{2016y / April / 24}) return false;
  if (sys_days{2024y / January / Monday[0]} != sys_days{2023y / December / 25}) return false;
  // the fifth weekday when the month has five of them
  if (!(2024y / March / Friday[5]).ok() || sys_days{2024y / March / Friday[5]} != sys_days{2024y / March / 29})
    return false;

  // extreme valid dates round-trip
  year_month_day lo = year::min() / January / 1;
  year_month_day hi = year::max() / December / 31;
  if (!lo.ok() || !hi.ok() || year::min() != year(-32767) || year::max() != year(32767)) return false;
  if (year_month_day{sys_days{lo}} != lo || year_month_day{sys_days{hi}} != hi) return false;
  // 65535 years from -32767 to 32767, 15891 of them leap years (proleptic Gregorian).
  if ((sys_days{hi} - sys_days{lo}).count() != 365LL * 65535 + 15891 - 1) return false;
  if (year_month_weekday{sys_days{hi}}.year() != year::max()) return false;

  // month arithmetic keeps an impossible day; conversion overflows into the next month.
  year_month_day a = 2023y / March / 31 - months(1);
  if (a != 2023y / February / 31 || a.ok() || sys_days{a} != sys_days{2023y / March / 3}) return false;
  year_month_day b = 2024y / January / 31 + months(-13);
  if (b != 2022y / December / 31 || !b.ok()) return false;
  year_month_day c = 2024y / August / 31 + months(1);
  if (c != 2024y / September / 31 || c.ok() || sys_days{c} != sys_days{2024y / October / 1}) return false;
  year_month_day d = 2024y / February / 29 + years(-1);
  if (d.ok() || sys_days{d} != sys_days{2023y / March / 1}) return false;

  // year_month_day_last follows the end of the month.
  year_month_day_last l = 2024y / January / last;
  if ((l + months(1)).day() != 29d || (l + months(13)).day() != 28d || (l - months(11)).day() != 28d) return false;
  if (year_month_day{l + months(3)} != 2024y / April / 30) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
