// operator<< of the composite calendar types when parts are not ok(). Each is specified as a
// format() call whose replacement fields format the parts; [time.format]/7: a part formatted
// with "{}" or "{:L}" (no chrono-specs) is formatted "as if by streaming it", so the part's own
// "... is not a valid ..." text appears inside the composite:
// [time.cal.wdidx.nonmembers]/2 "{:L}[{}]" for index 1-5, else "{:L}[{} is not a valid index]";
// [time.cal.wdlast.nonmembers] "{:L}[last]"; [time.cal.md.nonmembers]/3 "{:L}/{}";
// [time.cal.mdlast]/9 "{:L}/last"; [time.cal.mwd.nonmembers] and [time.cal.mwdlast.nonmembers]
// "{:L}/{:L}"; [time.cal.ym.nonmembers]/14 "{}/{:L}"; [time.cal.ymdlast.nonmembers]/12
// "{}/{:L}"; [time.cal.ymwd.nonmembers]/11 and [time.cal.ymwdlast.nonmembers]/11 "{}/{:L}/{:L}";
// [time.cal.day.nonmembers] "{:%d}" or "{:%d} is not a valid day"; [time.cal.month.nonmembers]
// "{} is not a valid month" (the unsigned value); [time.cal.year.nonmembers] "{:%Y} is not a
// valid year"; [time.cal.wd.nonmembers] "{} is not a valid weekday" (the unsigned c_encoding).
// [time.hms.nonmembers]: os << hms is format("{:L%T}", hms); [time.format]/3 (negative
// hh_mm_ss: "-" before the first conversion) and %S (fractional digits of the precision).
#include <chrono>
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
  // weekday_indexed: index 0 and 6 are not valid, index 1 and 5 are.
  CHECK(str(Monday[0]) == "Mon[0 is not a valid index]");
  CHECK(str(Monday[1]) == "Mon[1]");
  CHECK(str(Sunday[5]) == "Sun[5]");
  CHECK(str(Saturday[6]) == "Sat[6 is not a valid index]");
  CHECK(str(weekday(8)[2]) == "8 is not a valid weekday[2]");
  CHECK(str(weekday(9)[7]) == "9 is not a valid weekday[7 is not a valid index]");
  CHECK(str(weekday_last(weekday(10))) == "10 is not a valid weekday[last]");

  // month_day / month_day_last
  CHECK(str(month(13) / day(40)) == "13 is not a valid month/40 is not a valid day");
  CHECK(str(February / day(30)) == "Feb/30");  // the parts are ok, the combination is not
  CHECK(str(April / day(0)) == "Apr/00 is not a valid day");
  CHECK(str(month_day_last(month(0))) == "0 is not a valid month/last");

  // month_weekday / month_weekday_last
  CHECK(str(May / Tuesday[3]) == "May/Tue[3]");
  CHECK(str(month(14) / Tuesday[0]) == "14 is not a valid month/Tue[0 is not a valid index]");
  CHECK(str(June / Friday[last]) == "Jun/Fri[last]");
  CHECK(str(month_weekday_last(month(15), weekday_last(Friday))) == "15 is not a valid month/Fri[last]");

  // year_month / year_month_day_last
  CHECK(str(2024y / month(13)) == "2024/13 is not a valid month");
  CHECK(str(year(-32768) / March) == "-32768 is not a valid year/Mar");
  CHECK(str(2023y / February / last) == "2023/Feb/last");
  CHECK(str(year_month_day_last(2023y, month_day_last(month(13)))) == "2023/13 is not a valid month/last");

  // year_month_weekday / year_month_weekday_last
  CHECK(str(2024y / May / Tuesday[5]) == "2024/May/Tue[5]");  // ok() is false (May 2024 has 4), text is not
  CHECK(str(2024y / May / Tuesday[6]) == "2024/May/Tue[6 is not a valid index]");
  CHECK(str(2024y / month(0) / weekday(8)[1]) == "2024/0 is not a valid month/8 is not a valid weekday[1]");
  CHECK(str(2024y / May / Friday[last]) == "2024/May/Fri[last]");
  CHECK(str(year_month_weekday_last(2024y, month(13), weekday_last(Friday))) ==
        "2024/13 is not a valid month/Fri[last]");

  // year_month_day: the whole date is checked ("{:%F} is not a valid date").
  CHECK(str(2024y / April / 31) == "2024-04-31 is not a valid date");
  CHECK(str(2024y / month(13) / 1) == "2024-13-01 is not a valid date");
  CHECK(str(2024y / January / 0) == "2024-01-00 is not a valid date");

  // wide streams use the same texts
  std::wostringstream w;
  w << Monday[0] << L' ' << (month(13) / day(1));
  CHECK(w.str() == L"Mon[0 is not a valid index] 13 is not a valid month/01");

  // hh_mm_ss: "{:L%T}"; negative values get a leading '-'; %S shows the precision's digits.
  CHECK(str(hh_mm_ss<seconds>(seconds(3661))) == "01:01:01");
  CHECK(str(hh_mm_ss<seconds>(seconds(-3661))) == "-01:01:01");
  CHECK(str(hh_mm_ss<milliseconds>(milliseconds(-1500))) == "-00:00:01.500");
  CHECK(str(hh_mm_ss<microseconds>(microseconds(86'399'000'001))) == "23:59:59.000001");
  CHECK(str(hh_mm_ss<duration<int, std::centi>>(duration<int, std::centi>(-5))) == "-00:00:00.05");
  CHECK(str(hh_mm_ss<minutes>(minutes(-125))) == "-02:05:00");
  CHECK(str(hh_mm_ss<hours>(hours(7))) == "07:00:00");
  return 0;
}
