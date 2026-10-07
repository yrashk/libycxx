// chrono::parse: the E and O forms read the stream locale's alternative representations.
// [time.parse] Table 134: "%Ec interprets the locale's alternate date and time representation",
// %Ex / %EX "the locale's alternate date / time representation", "%EC interprets the locale's
// alternative representation of the century", "%Ey and %Oy interpret the locale's alternative
// representation", "%EY interprets the locale's alternative representation", and %Od (%Oe),
// %OH, %OI, %Om, %OM, %OS, %OU, %Ow, %OW "interpret the locale's alternative representation".
// ja_JP has eras (POSIX LC_TIME era: 令和, 平成, ... with "元年" for an era's first year) and
// alternative digits (〇 一 二 ... 九十九); my_MM has digits of its own script. The input is
// what the C library's strftime writes for the same locale (where a C library has no eras or
// digits for it, the plain forms, which must read back as well) and what chrono's own
// formatter writes with {:L%E...} and {:L%O...} (for a UTF-8 locale: [time.format]/3 converts
// the replacements of other locales to the literal encoding, which the locale cannot read).
// An era and a year within it combine ("%EC%Ey": year n of the era); %Ey without %EC is %y
// (POSIX strptime: "the offset from %EC", and %Oy "with the alternative digits").
#include <time.h>
#include <chrono>
#include <format>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

template <class C, class T>
static bool parses(const std::locale& loc, const std::basic_string<C>& in, const C* fmt, T& out) {
  std::basic_istringstream<C> is(in);
  is.imbue(loc);
  is >> parse(fmt, out);
  return !is.fail();
}

static std::tm tm_of(year_month_day ymd, hours h, minutes mi, seconds s) {
  std::tm t{};
  t.tm_year = int(ymd.year()) - 1900;
  t.tm_mon = int(unsigned(ymd.month())) - 1;
  t.tm_mday = int(unsigned(ymd.day()));
  t.tm_hour = int(h.count());
  t.tm_min = int(mi.count());
  t.tm_sec = int(s.count());
  t.tm_wday = int(weekday(sys_days(ymd)).c_encoding());
  t.tm_yday = int((sys_days(ymd) - sys_days(ymd.year() / January / 1)).count());
  return t;
}
static std::string c_ftime(const char* name, const char* fmt, const std::tm& t) {
  return in_c_locale(name, [&] {
    char b[512];
    const size_t n = strftime(b, sizeof b, fmt, &t);
    return std::string(b, n);
  });
}
static std::wstring c_wftime(const char* name, const wchar_t* fmt, const std::tm& t) {
  return in_c_locale(name, [&] {
    wchar_t b[512];
    const size_t n = wcsftime(b, 512, fmt, &t);
    return std::wstring(b, n);
  });
}

static void check_locale(const char* name) {
  const std::locale loc(name);
  const bool utf8 = in_c_locale(name, [] { return std::string(nl_langinfo(CODESET)); }) == "UTF-8";
  // what chrono's formatter writes, where the locale can read it back; else the C library's
  auto fmt = [&](const char* f, const std::string& c_text, sys_seconds tp) {
    return utf8 ? std::vformat(loc, f, std::make_format_args(tp)) : c_text;
  };
  // dates around the eras' boundaries (1989-01-08, 2019-05-01), an era's first year, and a
  // year of the Gregorian era of the locale
  const year_month_day dates[] = {2026y / October / 7, 2019y / June / 7, 2019y / April / 30, 1989y / October / 7,
                                  1989y / January / 7, 2000y / February / 29, 1926y / December / 25};
  for (const year_month_day ymd : dates) {
    const sys_seconds tp = sys_days(ymd) + 13h + 4min + 5s;
    const std::tm t = tm_of(ymd, 13h, 4min, 5s);
    // %EY: the full alternative year
    for (const std::string& s : {c_ftime(name, "%EY", t), fmt("{:L%EY}", c_ftime(name, "%EY", t), tp)}) {
      year y{};
      CHECK(parses(loc, s, "%EY", y) && y == ymd.year());
    }
    {
      year y{};
      CHECK(parses(loc, c_wftime(name, L"%EY", t), L"%EY", y) && y == ymd.year());
    }
    // %EC and %Ey together (in either order of the input's fields)
    {
      year y{};
      CHECK(parses(loc, c_ftime(name, "%EC %Ey", t), "%EC %Ey", y) && y == ymd.year());
      y = {};
      CHECK(parses(loc, c_ftime(name, "%Ey/%EC", t), "%Ey/%EC", y) && y == ymd.year());
    }
    // %Ex and %Ec
    for (const std::string& s : {c_ftime(name, "%Ex", t), fmt("{:L%Ex}", c_ftime(name, "%Ex", t), tp)}) {
      year_month_day got{};
      CHECK(parses(loc, s, "%Ex", got) && got == ymd);
    }
    {
      sys_seconds got{};
      if (utf8)
        CHECK(parses(loc, std::format(loc, "{:L%Ec}", tp), "%Ec", got) && got == tp);
      got = {};
      CHECK(parses(loc, std::format(loc, L"{:L%Ec}", tp), L"%Ec", got) && got == tp);
      seconds tod{};
      CHECK(parses(loc, c_ftime(name, "%EX", t), "%EX", tod) && tod == 13h + 4min + 5s);
    }
    // the O forms
    {
      const char* f = "%Y %Om %Od %OH %OM %OS";
      for (const std::string& s : {c_ftime(name, f, t), fmt("{:L%Y %Om %Od %OH %OM %OS}", c_ftime(name, f, t), tp)}) {
        sys_seconds got{};
        CHECK(parses(loc, s, f, got) && got == tp);
      }
      sys_seconds got{};
      CHECK(parses(loc, c_wftime(name, L"%Y %Om %Oe %OH %OM %OS", t), L"%Y %Om %Oe %OH %OM %OS", got) && got == tp);
      seconds twelve{};
      CHECK(parses(loc, c_ftime(name, "%OI %p", t), "%OI %p", twelve) && twelve == 13h);
      year_month_day d{};
      CHECK(parses(loc, c_ftime(name, "%Y %OU %Ow", t), "%Y %OU %Ow", d) && d == ymd);
      d = {};
      CHECK(parses(loc, c_ftime(name, "%Y %OW %Ow", t), "%Y %OW %Ow", d) && d == ymd);
      d = {};
      CHECK(parses(loc, c_ftime(name, "%G %OV %Ou", t), "%G %OV %Ou", d) && d == ymd);
      // %Oy: the last two digits, 1969-2068 without a century
      year y{};
      CHECK(parses(loc, c_ftime(name, "%Oy", t), "%Oy", y) && int(y) % 100 == int(ymd.year()) % 100);
      y = {};
      CHECK(parses(loc, c_ftime(name, "%C %Oy", t), "%C %Oy", y) && y == ymd.year());
    }
  }
  // %Ey without %EC is %y ([69, 99]: 1969-1999; [00, 68]: 2000-2068)
  {
    year y{};
    CHECK(parses(loc, std::string("26"), "%Ey", y) && y == 2026y);
    CHECK(parses(loc, std::string("76"), "%Ey", y) && y == 1976y);
  }
  // ASCII digits still read for an O form
  {
    day d{};
    CHECK(parses(loc, std::string("07"), "%Od", d) && d == day{7});
  }
  // an era name that is not one fails
  {
    year y{};
    CHECK(!parses(loc, std::string("XYZ 8"), "%EC %Ey", y));
  }
}

int main() {
  check_locale(require_locale("ja_JP.UTF-8"));
  for (const char* other : {"my_MM.UTF-8", "zh_TW.UTF-8", "ja_JP.eucJP"})
    if (c_has_locale(other))
      check_locale(other);
  return 0;
}
