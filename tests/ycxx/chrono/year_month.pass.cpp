// [time.cal.ym]: year_month(y, m); ok() is y_.ok() && m_.ok(); ym + months is the z with
// z.ok() && z - ym == dm; ym - ym is (x.year() - y.year()) + months{int(x.month()) -
// int(y.month())}; ym + years is (ym.year() + dy) / ym.month(); <=> orders by year then month.
#include <chrono>
#include <compare>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(std::is_same_v<decltype(2000y / January - 1999y / March), months>);
static_assert(std::is_same_v<decltype(2000y / January + months(1)), year_month>);

constexpr bool test() {
  year_month ym = 2000y / January;
  if (ym.year() != 2000y || ym.month() != January || !ym.ok()) return false;
  if ((year(-32768) / January).ok() || (2000y / month(13)).ok()) return false;
  if (ym + months(-13) != 1998y / December) return false;
  if (ym + months(11) != 2000y / December || ym + months(12) != 2001y / January) return false;
  if (months(25) + ym != 2002y / February || ym - months(1) != 1999y / December) return false;
  if (2001y / March - 2000y / May != months(10)) return false;
  if (2000y / May - 2001y / March != months(-10)) return false;
  if (ym + years(5) != 2005y / January || years(-5) + ym != 1995y / January || ym - years(1) != 1999y / January)
    return false;
  year_month z = ym;
  z += months(14);
  if (z != 2001y / March) return false;
  z -= years(2);
  if (z != 1999y / March) return false;
  z -= months(3);
  z += years(1);
  if (z != 1999y / December) return false;
  if (!(1999y / December < 2000y / January) || (ym <=> 2000y / January) != std::strong_ordering::equal) return false;
  // Large month offsets.
  if (ym + months(12 * 1000 + 5) != 3000y / June) return false;
  if (ym - months(12 * 2000 + 1) != year(-1) / December) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
