// [time.cal.ymwd]: operator sys_days is (index() - 1) * 7 days after the first weekday() of
// year()/month() (index 0: 7 days before it); construction from sys_days gives the year, month,
// weekday and its index; ok() is true when y_, m_, wdi_ are ok and the date is valid;
// [time.cal.ymwdlast]: operator sys_days is the last weekday() of year()/month(); arithmetic with
// months and years; == only.
#include <chrono>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

template <class T> concept ordered = requires(T a) { a < a; };
static_assert(!ordered<year_month_weekday> && !ordered<year_month_weekday_last>);
static_assert(std::is_convertible_v<sys_days, year_month_weekday>);
static_assert(!std::is_convertible_v<local_days, year_month_weekday>);

constexpr bool test() {
  year_month_weekday a = 2016y / May / Sunday[2];
  if (a.year() != 2016y || a.month() != May || a.weekday() != Sunday || a.index() != 2) return false;
  if (a.weekday_indexed() != Sunday[2] || !a.ok()) return false;
  if (sys_days{a} != sys_days{2016y / May / 8}) return false;  // second Sunday of May 2016
  if (sys_days{2016y / May / Sunday[1]} != sys_days{2016y / May / 1}) return false;
  if (sys_days{2016y / May / Sunday[0]} != sys_days{2016y / April / 24}) return false;
  if (sys_days{2016y / May / Sunday[5]} != sys_days{2016y / May / 29} || !(2016y / May / Sunday[5]).ok()) return false;
  if ((2016y / February / Sunday[5]).ok()) return false;  // Sundays 7, 14, 21, 28
  if ((2016y / May / Sunday[0]).ok() || (2016y / month(13) / Sunday[1]).ok()) return false;
  if (sys_days{1980y / January / Sunday[1]} != sys_days{1980y / January / 6}) return false;
  // From sys_days.
  year_month_weekday b{sys_days{2016y / May / 29}};
  if (b != 2016y / May / Sunday[5]) return false;
  year_month_weekday c{sys_days{2000y / January / 1}};
  if (c != 2000y / January / Saturday[1]) return false;
  if (year_month_weekday{local_days{2000y / January / 15}} != 2000y / January / Saturday[3]) return false;
  // Arithmetic keeps the weekday_indexed.
  if (a + months(1) != 2016y / June / Sunday[2] || months(8) + a != 2017y / January / Sunday[2]) return false;
  if (a - months(5) != 2015y / December / Sunday[2]) return false;
  if (a + years(1) != 2017y / May / Sunday[2] || a - years(1) != 2015y / May / Sunday[2]) return false;
  year_month_weekday d = a;
  d += months(12);
  d -= years(1);
  if (d != a) return false;

  year_month_weekday_last e = 2016y / May / Monday[last];
  if (e.year() != 2016y || e.month() != May || e.weekday() != Monday || e.weekday_last() != Monday[last]) return false;
  if (!e.ok() || sys_days{e} != sys_days{2016y / May / 30}) return false;
  if (sys_days{2016y / February / Monday[last]} != sys_days{2016y / February / 29}) return false;
  if (sys_days{2015y / February / Saturday[last]} != sys_days{2015y / February / 28}) return false;
  if ((year(-32768) / May / Monday[last]).ok() || (2016y / May / weekday(8)[last]).ok()) return false;
  if (e + months(1) != 2016y / June / Monday[last] || e - years(1) != 2015y / May / Monday[last]) return false;
  year_month_weekday_last f = e;
  f += years(2);
  f -= months(24);
  if (f != e) return false;
  if (local_days{e}.time_since_epoch() != sys_days{e}.time_since_epoch()) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // Round trip for every day of 1999-2001.
  for (sys_days d = sys_days{1999y / 1 / 1}; d < sys_days{2002y / 1 / 1}; d += days(1)) {
    year_month_weekday w{d};
    CHECK(w.ok());
    CHECK(sys_days{w} == d);
    CHECK(w.weekday() == weekday{d});
    CHECK(w.index() == (unsigned(year_month_day{d}.day()) - 1) / 7 + 1);
  }
}
