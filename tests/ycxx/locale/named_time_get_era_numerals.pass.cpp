// time_get of a named locale reads its eras and alternative digits.
// [locale.time.get.virtuals]/11: do_get "interprets ... characters from the sequence ... as
// strptime would" for the conversion (format, modifier); [locale.time.get.members]/8: get(fmt)
// calls do_get for each conversion. POSIX strptime: %Ec / %Ex / %EX "the locale's alternative
// date and time / date / time representation", %EC "the name of the base year (period) in the
// locale's alternative representation", %Ey "the offset from %EC (year only) in the locale's
// alternative representation", %EY "the full alternative year representation", and %Od %Oe %OH
// %OI %Om %OM %OS %OU %Ow %OW %Oy "using the locale's alternative numeric symbols".
// The input is the C library's strftime output for the same locale (ja_JP: eras such as 令和
// and 平成 with "元年" for an era's first year, and 〇 一 二 ... as digits); where the C library
// has no eras or digits for the locale, the plain forms, which must read back as well.
#include <time.h>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

template <class C>
static std::tm get(const std::locale& loc, const std::basic_string<C>& in, const std::basic_string<C>& fmt,
                   std::ios_base::iostate& err) {
  std::basic_istringstream<C> is(in);
  is.imbue(loc);
  std::tm t{};
  t.tm_year = -1000; // not set unless read
  err = std::ios_base::goodbit;
  std::use_facet<std::time_get<C>>(loc).get(std::istreambuf_iterator<C>(is), std::istreambuf_iterator<C>(), is, err, &t,
                                            fmt.data(), fmt.data() + fmt.size());
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
  struct date {
    int y, m, d;
  };
  // around the eras' boundaries, and an era's first year
  for (const date dt : {date{2026, 10, 7}, date{2019, 6, 7}, date{2019, 4, 30}, date{1989, 1, 7}, date{1989, 10, 8},
                        date{1926, 12, 25}}) {
    std::tm t{};
    t.tm_year = dt.y - 1900;
    t.tm_mon = dt.m - 1;
    t.tm_mday = dt.d;
    t.tm_hour = 13;
    t.tm_min = 4;
    t.tm_sec = 5;
    std::ios_base::iostate err;
    for (const char* f : {"%EY", "%EC%Ey", "%Ey %EC", "%Ex"}) {
      const std::tm r = get(loc, c_ftime(name, f, t), std::string(f), err);
      CHECK(!(err & std::ios_base::failbit) && r.tm_year == t.tm_year);
    }
    {
      const std::tm r = get(loc, c_wftime(name, L"%EY", t), std::wstring(L"%EY"), err);
      CHECK(!(err & std::ios_base::failbit) && r.tm_year == t.tm_year);
    }
    {
      const std::tm r = get(loc, c_ftime(name, "%Ec", t), std::string("%Ec"), err);
      CHECK(!(err & std::ios_base::failbit) && r.tm_year == t.tm_year && r.tm_mon == t.tm_mon && r.tm_mday == t.tm_mday &&
            r.tm_hour == 13 && r.tm_min == 4 && r.tm_sec == 5);
    }
    {
      const char* f = "%Od %Om %OH %OM %OS %Oy";
      const std::tm r = get(loc, c_ftime(name, f, t), std::string(f), err);
      CHECK(!(err & std::ios_base::failbit) && r.tm_mday == t.tm_mday && r.tm_mon == t.tm_mon && r.tm_hour == 13 &&
            r.tm_min == 4 && r.tm_sec == 5 && r.tm_year % 100 == t.tm_year % 100);
      const std::tm w = get(loc, c_wftime(name, L"%Oe %OI", t), std::wstring(L"%Oe %OI"), err);
      CHECK(!(err & std::ios_base::failbit) && w.tm_mday == t.tm_mday && w.tm_hour % 12 == 1);
    }
  }
  // one conversion: %EY with do_get directly
  {
    std::tm t{};
    t.tm_year = 2019 - 1900;
    t.tm_mon = 4;
    t.tm_mday = 1;
    const std::string s = c_ftime(name, "%EY", t);
    std::istringstream is(s);
    is.imbue(loc);
    std::ios_base::iostate err = std::ios_base::goodbit;
    std::tm r{};
    std::use_facet<std::time_get<char>>(loc).get(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>(), is,
                                                 err, &r, 'Y', 'E');
    CHECK(!(err & std::ios_base::failbit) && r.tm_year == 2019 - 1900);
  }
  // ASCII digits are still accepted by an O form
  {
    std::ios_base::iostate err;
    const std::tm r = get(loc, std::string("07"), std::string("%Od"), err);
    CHECK(!(err & std::ios_base::failbit) && r.tm_mday == 7);
  }
}

int main() {
  check_locale(require_locale("ja_JP.UTF-8"));
  for (const char* other : {"my_MM.UTF-8", "zh_TW.UTF-8", "ja_JP.eucJP"})
    if (c_has_locale(other))
      check_locale(other);
  return 0;
}
