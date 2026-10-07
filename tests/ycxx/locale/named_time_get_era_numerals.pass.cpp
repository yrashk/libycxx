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
// has no eras or digits for the locale, the plain forms, which must read back as well (POSIX:
// where the alternative representation is not available, the unmodified one is used). Whether
// it has them is asked of the C library at run time (strftime's %EC against %C, %Ec against
// %c), never assumed from glibc's data; de_DE (no eras, no digits anywhere) checks that case.
#include <langinfo.h>
#include <time.h>
#include <wchar.h>
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
// A zone for strftime's %Z (de_DE's %c shows it), where the C library's tm has the BSD members
// (glibc and Darwin do); without one %Z may be empty, and nothing would be there to read.
template <class TM>
static void set_utc(TM& t) {
  if constexpr (requires { t.tm_zone; t.tm_gmtoff; }) {
    t.tm_zone = const_cast<decltype(t.tm_zone)>("UTC");
    t.tm_gmtoff = 0;
  }
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
    t.tm_isdst = 0;
    set_utc(t);
    {
      // the weekday and day of the year, which %c may show: from the C library
      std::tm n = t;
      n.tm_hour = 12;
      timegm(&n);
      set_utc(t);
      t.tm_wday = n.tm_wday;
      t.tm_yday = n.tm_yday;
    }
    std::ios_base::iostate err;
    if (c_ftime(name, "%EC", t) == c_ftime(name, "%C", t)) {
      // no era for this date in the C library: the E forms are the unmodified ones
      CHECK(c_ftime(name, "%EY", t) == c_ftime(name, "%Y", t));
    }
    if (c_ftime(name, "%Ec", t) == c_ftime(name, "%c", t)) {
      // %Ec reads what %c reads
      const std::string text = c_ftime(name, "%c", t);
      const std::string d_t_fmt = in_c_locale(name, [] { return std::string(nl_langinfo(D_T_FMT)); });
      const std::string era_d_t_fmt = in_c_locale(name, [] { return std::string(nl_langinfo(ERA_D_T_FMT)); });
      const std::tm a = get(loc, text, std::string("%Ec"), err);
      std::ios_base::iostate err2;
      const std::tm b = get(loc, text, std::string("%c"), err2);
      CHECK_SAY(!(err & std::ios_base::failbit), "locale %s, D_T_FMT \"%s\", ERA_D_T_FMT \"%s\", text \"%s\"; %%c %s",
                name, d_t_fmt.c_str(), era_d_t_fmt.c_str(), text.c_str(),
                (err2 & std::ios_base::failbit) ? "failed too" : "read it");
      CHECK(!(err2 & std::ios_base::failbit) && a.tm_year == b.tm_year && a.tm_mon == b.tm_mon && a.tm_mday == b.tm_mday &&
            a.tm_hour == b.tm_hour && a.tm_min == b.tm_min && a.tm_sec == b.tm_sec);
    }
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
  for (const char* other : {"de_DE.UTF-8", "my_MM.UTF-8", "zh_TW.UTF-8", "ja_JP.eucJP"})
    if (c_has_locale(other))
      check_locale(other);
  return 0;
}
