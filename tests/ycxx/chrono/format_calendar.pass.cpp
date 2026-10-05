// [time.cal.*.nonmembers] operator<<: day "{:%d}" or "{:%d} is not a valid day"; month "{:L%b}"
// or "{} is not a valid month"; year "{:%Y}" or "{:%Y} is not a valid year"; weekday "{:L%a}"
// or "{} is not a valid weekday"; year_month_day "{:%F}" or "{:%F} is not a valid date";
// month_day "{:L}/{}"; year_month "{}/{:L}"; weekday_indexed "{:L}[{}]" or "{:L}[{} is not a
// valid index]"; weekday_last "{:L}[last]"; year_month_day_last "{}/{:L}" (month_day_last is
// "{:L}/last"); year_month_weekday "{}/{:L}/{:L}". [time.clock.system.nonmembers]/4:
// os << sys_days is os << year_month_day{dp}.
// [time.format] Table 133 in the "C" locale ([time.format]/2: without L the "C" locale is
// used): %a %A %b %B %C %d %D %e %F %g %G %j %m %u %U %V %w %W %y %Y; [time.format]/3: a
// specifier for missing information (%d for a year_month, %u for a weekday that is not ok())
// throws format_error.
// REQUIRES: exceptions
#include <chrono>
#include <format>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;

template <class T>
static std::string str(const T& t) {
  std::ostringstream os;
  os << t;
  return os.str();
}

int main() {
  CHECK(str(day(5)) == "05");
  CHECK(str(day(0)) == "00 is not a valid day");
  CHECK(str(day(32)) == "32 is not a valid day");
  CHECK(str(month(1)) == "Jan");
  CHECK(str(December) == "Dec");
  CHECK(str(month(13)) == "13 is not a valid month");
  CHECK(str(year(2024)) == "2024");
  CHECK(str(year(33)) == "0033");
  CHECK(str(year(-1)) == "-0001");
  CHECK(str(year::min()) == "-32767");
  CHECK(str(year(-32768)) == "-32768 is not a valid year");
  CHECK(str(Sunday) == "Sun");
  CHECK(str(weekday(6)) == "Sat");
  CHECK(str(weekday(7)) == "Sun");  // weekday(7) is Sunday
  CHECK(str(weekday(8)) == "8 is not a valid weekday");
  CHECK(str(2024y / February / 29) == "2024-02-29");
  CHECK(str(2023y / February / 29) == "2023-02-29 is not a valid date");
  CHECK(str(year_month_day(sys_days(days(0)))) == "1970-01-01");
  CHECK(str(sys_days(days(19723))) == "2024-01-01");
  CHECK(str(March / 7) == "Mar/07");
  CHECK(str(2024y / April) == "2024/Apr");
  CHECK(str(Monday[2]) == "Mon[2]");
  CHECK(str(Monday[6]) == "Mon[6 is not a valid index]");
  CHECK(str(Friday[last]) == "Fri[last]");
  CHECK(str(February / last) == "Feb/last");
  CHECK(str(2024y / February / last) == "2024/Feb/last");
  CHECK(str(2024y / May / Tuesday[3]) == "2024/May/Tue[3]");

  // conversion specifiers ("C" locale)
  const year_month_day d = 2024y / March / 5;  // a Tuesday, day 65 of a leap year
  CHECK(std::format("{:%F}", d) == "2024-03-05");
  CHECK(std::format("{:%Y/%m/%d}", d) == "2024/03/05");
  CHECK(std::format("{:%D}", d) == "03/05/24");
  CHECK(std::format("{:%e}", d) == " 5");
  CHECK(std::format("{:%a %A}", d) == "Tue Tuesday");
  CHECK(std::format("{:%b %B %h}", d) == "Mar March Mar");
  CHECK(std::format("{:%C %y}", d) == "20 24");
  CHECK(std::format("{:%j}", d) == "065");
  CHECK(std::format("{:%u %w}", d) == "2 2");
  CHECK(std::format("{:%u %w}", 2024y / March / 3) == "7 0");  // Sunday
  CHECK(std::format("{:%U %W %V}", d) == "09 10 10");
  // ISO week-based year around a year boundary: 2021-01-01 is in week 53 of 2020
  CHECK(std::format("{:%G %g %V}", 2021y / January / 1) == "2020 20 53");
  CHECK(std::format("{:%G-W%V-%u}", 2024y / December / 30) == "2025-W01-1");
  CHECK(std::format("{:%U %W}", 2023y / January / 1) == "01 00");  // a Sunday
  CHECK(std::format("{:%Y}", year(-44)) == "-0044");
  CHECK(std::format("{:%C %y}", year(-1976)) == "-20 76");  // [time.format] Example 3
  CHECK(std::format("{:%d}", day(7)) == "07");
  CHECK(std::format("{:%B}", month(10)) == "October");
  CHECK(std::format("{:%A}", weekday(4)) == "Thursday");
  CHECK(std::format("{:%m}", month(2)) == "02");
  CHECK(std::format("{:%Y-%m}", 2024y / July) == "2024-07");
  CHECK(std::format("{:%F}", 2024y / February / last) == "2024-02-29");
  CHECK(std::format("{:%F}", 2024y / May / Tuesday[3]) == "2024-05-21");
  CHECK(std::format("{}", 2024y / February / 29) == "2024-02-29");
  CHECK(std::format("{:>12}", 2024y / February / 29) == "  2024-02-29");
  CHECK(std::format("{}", 2023y / February / 29) == "2023-02-29 is not a valid date");
  // missing information
  bool thrown = false;
  try {
    year_month ym = 2024y / July;
    (void)std::vformat("{:%d}", std::make_format_args(ym));
  } catch (const std::format_error&) {
    thrown = true;
  }
  CHECK(thrown);
  thrown = false;
  try {
    month m(13);
    (void)std::vformat("{:%b}", std::make_format_args(m));  // %b of an invalid month
  } catch (const std::format_error&) {
    thrown = true;
  }
  CHECK(thrown);
  // a weekday that is not ok() has no ISO or C weekday number either ([time.format]/3; libc++'s
  // invalid-value tests expect format_error, libstdc++'s the encoding)
  for (const char* spec : {"{:%u}", "{:%w}", "{:%a}", "{:L%Ow}"}) {
    thrown = false;
    try {
      weekday wd(8);
      (void)std::vformat(spec, std::make_format_args(wd));
    } catch (const std::format_error&) {
      thrown = true;
    }
    CHECK(thrown);
  }
  thrown = false;
  try {
    year_month_day ymd = 2023y / February / 30;
    (void)std::vformat("{:%w}", std::make_format_args(ymd));
  } catch (const std::format_error&) {
    thrown = true;
  }
  CHECK(thrown);
  // wide
  std::wostringstream w;
  w << (2024y / February / 29) << L' ' << Monday << L' ' << March;
  CHECK(w.str() == L"2024-02-29 Mon Mar");
  return 0;
}
