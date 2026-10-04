// [time.cal.ymd.members]: year_month_day(sys_days) gives the date dp represents, and operator
// sys_days gives the count of days from the epoch; "A sys_days in the range [days{-12687428},
// days{11248737}] which is converted to a year_month_day has the same value when converted back to
// a sys_days." Checked for every day of that range against a date counter that steps through the
// proleptic Gregorian calendar one day at a time from 1970-01-01.
#include <chrono>
#include "check.hpp"

using namespace std::chrono;

static int days_in(int y, unsigned m) {
  static const unsigned char len[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  bool leap = y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
  return m == 2 && leap ? 29 : len[m];
}

int main() {
  // Forward from the epoch.
  int y = 1970;
  unsigned m = 1, d = 1;
  for (int n = 0; n <= 11248737; ++n) {
    year_month_day ymd{sys_days{days(n)}};
    CHECK(int(ymd.year()) == y && unsigned(ymd.month()) == m && unsigned(ymd.day()) == d);
    CHECK(ymd.ok());
    CHECK(sys_days{ymd}.time_since_epoch().count() == n);
    if (int(d) == days_in(y, m)) {
      d = 1;
      if (++m == 13) { m = 1; ++y; }
    } else {
      ++d;
    }
  }
  CHECK(y == 32768 && m == 1 && d == 1);
  // Backward from the epoch.
  y = 1970, m = 1, d = 1;
  for (int n = 0; n >= -12687428; --n) {
    year_month_day ymd{sys_days{days(n)}};
    CHECK(int(ymd.year()) == y && unsigned(ymd.month()) == m && unsigned(ymd.day()) == d);
    CHECK(sys_days{ymd}.time_since_epoch().count() == n);
    if (d == 1) {
      if (--m == 0) { m = 12; --y; }
      d = unsigned(days_in(y, m));
    } else {
      --d;
    }
  }
  CHECK(y == -32768 && m == 12 && d == 31);
}
