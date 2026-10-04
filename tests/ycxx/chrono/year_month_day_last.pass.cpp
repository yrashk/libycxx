// [time.cal.ymdlast]: year_month_day_last(y, mdl); year(), month(), month_day_last(); day() is the
// last day of the month when ok(); operator sys_days is sys_days{year()/month()/day()}; ok() is
// y_.ok() && mdl_.ok(); ymdl + months is (year()/month() + dm)/last, + years keeps the month;
// == and <=> compare year then month_day_last.
#include <chrono>
#include <compare>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(std::is_same_v<decltype(2000y / February / last), year_month_day_last>);
static_assert(std::is_same_v<decltype(2000y / February / last <=> 2000y / March / last), std::strong_ordering>);
static_assert(std::is_convertible_v<year_month_day_last, sys_days>);
static_assert(!std::is_convertible_v<year_month_day_last, local_days>);

constexpr bool test() {
  constexpr unsigned len[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  for (unsigned m = 1; m <= 12; ++m) {
    year_month_day_last l = 2023y / month(m) / last;
    if (!l.ok() || unsigned(l.day()) != len[m - 1] || l.month() != month(m) || l.year() != 2023y) return false;
    if (l.month_day_last() != month(m) / last) return false;
  }
  if ((2024y / February / last).day() != 29d || (2000y / February / last).day() != 29d) return false;
  if ((1900y / February / last).day() != 28d) return false;
  if ((year(-32768) / January / last).ok() || (2000y / month(0) / last).ok()) return false;
  if (sys_days{2000y / February / last} != sys_days{2000y / February / 29}) return false;
  if (local_days{2000y / February / last}.time_since_epoch() != sys_days{2000y / February / 29}.time_since_epoch())
    return false;
  year_month_day_last l = 2000y / January / last;
  if (l + months(1) != 2000y / February / last || months(13) + l != 2001y / February / last) return false;
  if (l - months(1) != 1999y / December / last) return false;
  if (l + years(1) != 2001y / January / last || years(1) + l != 2001y / January / last || l - years(1) != 1999y / January / last)
    return false;
  l += months(1);
  if (l.day() != 29d) return false;
  l += years(1);
  if (l.day() != 28d || l != 2001y / February / last) return false;
  l -= months(2);
  l -= years(1);
  if (l != 1999y / December / last) return false;
  if (!(2000y / January / last < 2000y / February / last) || !(1999y / December / last < 2000y / January / last))
    return false;
  if (year_month_day_last(2000y, month_day_last(March)) != 2000y / March / last) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
