// [time.cal.ymd]: ok() when y_ and m_ are ok and d_ is in [1d, (y_/m_/last).day()]; conversion to
// sys_days for ok dates, and for !ok dates with valid year and month sys_days{y_/m_/1d} + (d_ - 1d)
// (the draft's examples: 2017y/January/0 -> 2016-12-31, January/32 -> February/1); ymd + months is
// (ymd.year()/ymd.month() + dm)/ymd.day(); ymd + years is (ymd.year() + dy)/ymd.month()/ymd.day();
// construction from year_month_day_last and local_days; == and <=>.
#include <chrono>
#include <compare>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(std::is_convertible_v<sys_days, year_month_day>);
static_assert(!std::is_convertible_v<local_days, year_month_day> && std::is_constructible_v<year_month_day, local_days>);
static_assert(std::is_convertible_v<year_month_day, sys_days>);
static_assert(!std::is_convertible_v<year_month_day, local_days> && std::is_constructible_v<local_days, year_month_day>);
static_assert(std::is_convertible_v<year_month_day_last, year_month_day>);
static_assert(std::is_same_v<decltype(2000y / 1 / 1 <=> 2000y / 1 / 2), std::strong_ordering>);

// The draft's examples ([time.cal.ymd.members]/14).
static_assert(year_month_day{sys_days{2017y / January / 0}} == 2016y / December / 31);
static_assert(year_month_day{sys_days{2017y / January / 31}} == 2017y / January / 31);
static_assert(year_month_day{sys_days{2017y / January / 32}} == 2017y / February / 1);

constexpr bool test() {
  year_month_day a(2016y, May, 8d);
  if (a.year() != 2016y || a.month() != May || a.day() != 8d || !a.ok()) return false;
  if (!(2000y / February / 29).ok() || (1900y / February / 29).ok() || !(2024y / February / 29).ok()) return false;
  if ((2023y / February / 29).ok() || (2023y / April / 31).ok() || (2023y / April / 0).ok()) return false;
  if ((2023y / month(13) / 1).ok() || (year(-32768) / 1 / 1).ok() || !(year(-32767) / 1 / 1).ok()) return false;
  // Not-ok dates convert with the overflow rule.
  if (sys_days{2017y / February / 31} != sys_days{2017y / March / 3}) return false;
  if (sys_days{2016y / February / 30} != sys_days{2016y / March / 1}) return false;
  if (sys_days{2017y / March / 0} != sys_days{2017y / February / 28}) return false;
  // Month arithmetic keeps the day.
  year_month_day b = 2017y / January / 31 + months(1);
  if (b != 2017y / February / 31 || b.ok()) return false;
  if (2017y / January / 15 - months(1) != 2016y / December / 15) return false;
  if (months(13) + 2017y / January / 15 != 2018y / February / 15) return false;
  // Year arithmetic.
  year_month_day c = 2016y / February / 29 + years(1);
  if (c != 2017y / February / 29 || c.ok()) return false;
  if (2016y / February / 29 + years(4) != 2020y / February / 29) return false;
  if (years(-16) + 2016y / March / 1 != 2000y / March / 1 || 2016y / March / 1 - years(16) != 2000y / March / 1)
    return false;
  year_month_day d = 2000y / January / 1;
  d += months(25);
  d -= years(1);
  d += years(3);
  d -= months(2);
  if (d != 2003y / December / 1) return false;
  // From year_month_day_last and to/from local_days.
  if (year_month_day{2023y / February / last} != 2023y / February / 28) return false;
  if (year_month_day{2024y / February / last} != 2024y / February / 29) return false;
  if (year_month_day{local_days{days(10957)}} != 2000y / January / 1) return false;
  if (local_days{2000y / January / 1}.time_since_epoch() != days(10957)) return false;
  // Ordering: year, then month, then day.
  if (!(1999y / December / 31 < 2000y / January / 1) || !(2000y / January / 31 < 2000y / February / 1)) return false;
  if (!(2000y / March / 1 > 2000y / February / 29)) return false;
  if ((2000y / March / 1 <=> 2000y / March / 1) != std::strong_ordering::equal) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
