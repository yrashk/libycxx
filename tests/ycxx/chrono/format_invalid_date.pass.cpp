// [time.format]/3: "If the formatted object does not contain the information the conversion
// specifier refers to, an exception of type format_error is thrown." A year_month_day,
// year_month_day_last, year_month_weekday or year_month_weekday_last that is not ok() names no
// day, so the specifiers that need one (%j, %U, %W, %V, %G, %g and their O forms) throw, as do
// the weekday ones (%a, %A, %u, %w) unless the value holds a weekday of its own; the fields it
// does hold (%Y, %m, %d, %F, %B for a valid month) are still formatted.
// REQUIRES: exceptions
#include <chrono>
#include <format>
#include <locale>
#include <string>
#include "check.hpp"

using namespace std::chrono;

template <class T>
static bool throws(std::string_view fmt, const T& v) {
  try {
    (void)std::vformat(fmt, std::make_format_args(v));
  } catch (const std::format_error&) {
    return true;
  }
  return false;
}

template <class T>
static bool throws_L(std::string_view fmt, const T& v) {
  try {
    (void)std::vformat(std::locale::classic(), fmt, std::make_format_args(v));
  } catch (const std::format_error&) {
    return true;
  }
  return false;
}

int main() {
  const year_month_day bad[] = {
      {year(1970), month(1), day(0)},  {year(1970), month(1), day(32)},    {year(1970), month(2), day(29)},
      {year(1970), month(0), day(31)}, {year(-32768), month(1), day(31)},
  };
  for (const char* f : {"{:%j}", "{:%U}", "{:%W}", "{:%V}", "{:%G}", "{:%g}", "{:%a}", "{:%A}", "{:%u}", "{:%w}",
                        "{:%OU}", "{:%OW}", "{:%OV}", "{:%Ou}", "{:%Ow}"}) {
    for (const auto& d : bad) {
      CHECK(throws(f, d));
      CHECK(throws_L(std::string(f).insert(2, "L"), d));
    }
    CHECK(throws(f, year_month_day_last(year(1970), month_day_last(month(13)))));
  }
  // a year_month_weekday names its weekday even when it names no day
  for (const char* f : {"{:%j}", "{:%U}", "{:%W}", "{:%V}", "{:%G}", "{:%g}", "{:%OU}"}) {
    CHECK(throws(f, year_month_weekday(year(1970), month(1), weekday_indexed(Monday, 5))));
    CHECK(throws(f, year_month_weekday_last(year(1970), month(13), weekday_last(Monday))));
  }
  CHECK(std::format("{:%a %u}", year_month_weekday(year(1970), month(1), weekday_indexed(Monday, 5))) == "Mon 1");
  // the fields an invalid date holds
  const year_month_day d(year(1970), month(2), day(30));
  CHECK(std::format("{:%Y %m %d %F %B}", d) == "1970 02 30 1970-02-30 February");
  CHECK(std::format("{}", d) == "1970-02-30 is not a valid date");
  // a valid date and a duration's %j are unaffected
  CHECK(std::format("{:%j %U %W %V %G %g %u %w}", year_month_day(year(1970), month(3), day(1))) ==
        "060 09 08 09 1970 70 7 0");
  CHECK(std::format("{:%j}", days(400)) == "400");
  CHECK(std::format("{:%j}", year_month_weekday(year(1970), month(1), weekday_indexed(Monday, 1))) == "005");
  return 0;
}
