// [time.cal.operators]/5-44: the value each operator/ returns, in constant evaluation: every int
// operand goes through the year, month or day constructor (so 2015y / 13 holds month(13) and is
// not ok(), April / 31 holds day(31)), and every order of the operands gives the same object.
// Each operator is noexcept.
#include <chrono>
#include "check.hpp"

using namespace std::chrono;

static_assert(noexcept(2015y / 4) && noexcept(4 / last) && noexcept(Monday[2] / 3) && noexcept(2020 / (May / 1d)));
static_assert(noexcept(2015y / April / last) && noexcept(Friday[last] / 7 / 2001));

constexpr bool run() {
  // /5-6
  year_month ym = 2015y / April;
  if (ym.year() != 2015y || ym.month() != April) return false;
  if (2015y / 4 != ym) return false;
  if ((2015y / 13).month() != month(13) || (2015y / 13).ok()) return false;
  // /7-11: month_day in all orders.
  month_day md = April / 4d;
  if (md.month() != April || md.day() != 4d) return false;
  if (April / 4 != md || 4 / 4d != md || 4d / April != md || 4d / 4 != md) return false;
  if ((April / 31).day() != day(31) || (April / 31).ok() || !(May / 31).ok()) return false;
  // /12-15: month_day_last.
  if ((April / last).month() != April || 4 / last != April / last || last / April != April / last ||
      last / 4 != April / last)
    return false;
  // /16-19: month_weekday.
  month_weekday mw = March / Tuesday[2];
  if (mw.month() != March || mw.weekday_indexed() != Tuesday[2]) return false;
  if (3 / Tuesday[2] != mw || Tuesday[2] / March != mw || Tuesday[2] / 3 != mw) return false;
  // /20-23: month_weekday_last.
  month_weekday_last mwl = March / Sunday[last];
  if (mwl.month() != March || mwl.weekday_last() != Sunday[last]) return false;
  if (3 / Sunday[last] != mwl || Sunday[last] / March != mwl || Sunday[last] / 3 != mwl) return false;

  // /24-29: year_month_day from the three orders.
  year_month_day ymd = 2015y / April / 4d;
  if (ymd.year() != 2015y || ymd.month() != April || ymd.day() != 4d) return false;
  if (2015y / April / 4 != ymd || 2015y / (April / 4d) != ymd || 2015 / (April / 4d) != ymd) return false;
  if (April / 4d / 2015y != ymd || April / 4 / 2015 != ymd || 4d / April / 2015 != ymd || 4d / 4 / 2015 != ymd)
    return false;
  if (ym / 4 != ymd || ym / 4d != ymd) return false;
  if ((2015 / (February / 29)).ok() || !(2016 / (February / 29)).ok()) return false;
  if ((-44 / (March / 15d)).year() != year(-44)) return false;
  // /30-34: year_month_day_last.
  year_month_day_last ymdl = 2016y / February / last;
  if (ymdl.year() != 2016y || ymdl.month() != February || ymdl.day() != 29d) return false;
  if (ymdl.month_day_last() != February / last) return false;
  if (2016y / (February / last) != ymdl || 2016 / (February / last) != ymdl) return false;
  if (February / last / 2016y != ymdl || February / last / 2016 != ymdl || last / 2 / 2016 != ymdl) return false;
  // /35-39: year_month_weekday.
  year_month_weekday ymw = 2026y / October / Tuesday[1];
  if (ymw.year() != 2026y || ymw.month() != October || ymw.weekday_indexed() != Tuesday[1]) return false;
  if (2026y / (October / Tuesday[1]) != ymw || 2026 / (10 / Tuesday[1]) != ymw) return false;
  if (October / Tuesday[1] / 2026y != ymw || Tuesday[1] / 10 / 2026 != ymw) return false;
  if (year_month_day(ymw) != 2026y / October / 6d) return false; // 2026-10-06 is the first Tuesday
  // /40-44: year_month_weekday_last.
  year_month_weekday_last ymwl = 2026y / October / Saturday[last];
  if (ymwl.year() != 2026y || ymwl.month() != October || ymwl.weekday_last() != Saturday[last]) return false;
  if (2026y / (October / Saturday[last]) != ymwl || 2026 / (October / Saturday[last]) != ymwl) return false;
  if (October / Saturday[last] / 2026y != ymwl || Saturday[last] / 10 / 2026 != ymwl) return false;
  if (year_month_day(ymwl) != 2026y / October / 31d) return false;
  // Note 1's precedence example: 2015/4/4 is int division.
  static_assert(2015 / 4 / 4 == 125);
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
