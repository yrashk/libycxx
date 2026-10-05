// [locale.time.put.virtuals]/1: do_put formats as strftime does. ISO C describes %Y ("the year
// as a decimal number (e.g., 1997)") and %G ("the week-based year as a decimal number (e.g.,
// 1997)") alike, so a date whose week-based year is its calendar year gives the same text for
// both, whatever the number of digits.
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

static std::string put(const std::tm& t, const char* fmt) {
  std::ostringstream os;
  os << std::put_time(&t, fmt);
  return os.str();
}

int main() {
  for (int year : {5, 42, 999, 1997, 12345}) {
    std::tm t{};
    t.tm_year = year - 1900;
    t.tm_mon = 5; // June 15: the week-based year is the calendar year, whatever the weekday
    t.tm_mday = 15;
    t.tm_yday = 165;
    t.tm_wday = 3;
    CHECK(put(t, "%G") == put(t, "%Y"));
    CHECK(put(t, "%Y") == std::to_string(year));
  }
  return 0;
}
