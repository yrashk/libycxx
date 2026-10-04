// [time.cal.ym.members] etc.: the months overloads are constrained so that "If the argument
// supplied by the caller for the months parameter is convertible to years, its implicit conversion
// sequence to years is worse than its implicit conversion sequence to months". So an argument
// convertible to both (for example decades) selects the years overload, while months and years
// select their own overloads.
#include <chrono>
#include <ratio>
#include "check.hpp"

using namespace std::chrono;
using decades = duration<int, std::ratio_multiply<std::ratio<10>, years::period>>;

constexpr bool test() {
  year_month_day d = 2000y / January / 31;
  if (d + years(1) != 2001y / January / 31) return false;
  if (d + months(1) != 2000y / February / 31) return false;
  if (d + decades(1) != 2010y / January / 31) return false;  // years overload
  if (d - decades(2) != 1980y / January / 31) return false;
  year_month ym = 2000y / March;
  if (ym + decades(1) != 2010y / March || decades(1) + ym != 2010y / March) return false;
  ym += decades(1);
  if (ym != 2010y / March) return false;
  year_month_day_last l = 2000y / February / last;
  if ((l + decades(1)).day() != 28d) return false;  // 2010: still February
  year_month_weekday w = 2000y / May / Sunday[2];
  if (w + decades(1) != 2010y / May / Sunday[2]) return false;
  year_month_weekday_last wl = 2000y / May / Sunday[last];
  if (wl + decades(1) != 2010y / May / Sunday[last]) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
