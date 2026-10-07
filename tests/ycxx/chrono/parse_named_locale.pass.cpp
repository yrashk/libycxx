// chrono::parse reads the locale-dependent flags in the stream's locale.
// [time.parse] Table 134: %a "The locale's full or abbreviated case-insensitive weekday name",
// %b likewise for months (%A, %B, %h are equivalent), %c "The locale's date and time
// representation", %x "The locale's date representation", %X "The locale's time
// representation", %p "The locale's equivalent of the AM/PM designations associated with a
// 12-hour clock", %r "The locale's 12-hour clock time". [time.parse]/1 and /15: from_stream
// reads is, so "the locale" is is.getloc(). [time.parse]/15: a white-space character of the
// format matches zero or more white-space characters.
// The expected names and representations are the C library's own for the same locale
// (nl_langinfo, strftime), and the text chrono's own formatter writes with {:L%c} & co.
// ([time.format]: the same "locale's ... representation"), so both must read back.
// A char stream compares the names through ctype<char>::tolower (an ASCII change of case is
// ignored); a wchar_t stream through ctype<wchar_t>::tolower (any).
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <chrono>
#include <format>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

template <class C, class T, class... Extra>
static bool parses(const std::locale& loc, const std::basic_string<C>& in, const C* fmt, T& out, Extra&... extra) {
  std::basic_istringstream<C> is(in);
  is.imbue(loc);
  is >> parse(fmt, out, extra...);
  return !is.fail();
}

static std::tm tm_of(int y, int mon, int d, int h, int mi, int s, int wday, int yday) {
  std::tm t{};
  t.tm_year = y - 1900;
  t.tm_mon = mon - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = mi;
  t.tm_sec = s;
  t.tm_wday = wday;
  t.tm_yday = yday;
  return t;
}

// strftime / wcsftime in the C library's locale `name`.
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
static std::string c_langinfo(const char* name, nl_item item) {
  return in_c_locale(name, [&] { return std::string(nl_langinfo(item)); });
}
static std::string ascii_upper(std::string s) {
  for (char& c : s)
    if (c >= 'a' && c <= 'z')
      c = char(c - 'a' + 'A');
  return s;
}
static std::wstring wide_upper(const char* name, std::wstring s) {
  return in_c_locale(name, [&] {
    for (wchar_t& c : s)
      c = wchar_t(towupper(wint_t(c)));
    return s;
  });
}

static void check_locale(const char* name) {
  const std::locale loc(name);
  static const nl_item mon[12] = {MON_1, MON_2, MON_3, MON_4, MON_5, MON_6, MON_7, MON_8, MON_9, MON_10, MON_11, MON_12};
  static const nl_item abmon[12] = {ABMON_1, ABMON_2, ABMON_3, ABMON_4,  ABMON_5,  ABMON_6,
                                    ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12};
  static const nl_item day[7] = {DAY_1, DAY_2, DAY_3, DAY_4, DAY_5, DAY_6, DAY_7};
  static const nl_item abday[7] = {ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7};
  // %b %B %h: full and abbreviated month names, in any ASCII case
  for (unsigned m = 1; m <= 12; ++m) {
    for (const std::string& n : {c_langinfo(name, mon[m - 1]), c_langinfo(name, abmon[m - 1])}) {
      month got{};
      CHECK(parses(loc, n, "%b", got) && got == month{m});
      got = month{};
      CHECK(parses(loc, ascii_upper(n), "%B", got) && got == month{m});
      got = month{};
      CHECK(parses(loc, n, "%h", got) && got == month{m});
    }
  }
  // %a %A: weekday names
  for (unsigned d = 0; d < 7; ++d) {
    for (const std::string& n : {c_langinfo(name, day[d]), c_langinfo(name, abday[d])}) {
      weekday got{};
      CHECK(parses(loc, n, "%a", got) && got == weekday{d});
      got = weekday{};
      CHECK(parses(loc, ascii_upper(n), "%A", got) && got == weekday{d});
    }
  }

  const sys_seconds tp = sys_days{2026y / October / 7} + 13h + 4min + 5s; // a Wednesday
  const std::tm t = tm_of(2026, 10, 7, 13, 4, 5, 3, 279);

  // %c: chrono's own {:L%c} (a sys_time's zone is UTC: a %Z in the locale's %c reads it back)
  {
    const std::string s = std::format(loc, "{:L%c}", tp);
    sys_seconds got{};
    const std::string d_t_fmt = c_langinfo(name, D_T_FMT);
    CHECK_SAY((parses(loc, s, "%c", got) && got == tp), "locale %s, D_T_FMT \"%s\", text \"%s\"", name,
              d_t_fmt.c_str(), s.c_str());
    std::string abbrev;
    got = {};
    CHECK_SAY((parses(loc, s, "%c", got, abbrev) && got == tp), "locale %s, D_T_FMT \"%s\", text \"%s\"", name,
              d_t_fmt.c_str(), s.c_str());
    const std::wstring w = std::format(loc, L"{:L%c}", tp);
    got = {};
    CHECK(parses(loc, w, L"%c", got) && got == tp);
  }
  // %x: chrono's {:L%x} and the C library's strftime("%x")
  for (const std::string& s : {std::format(loc, "{:L%x}", tp), c_ftime(name, "%x", t)}) {
    year_month_day got{};
    // a two-digit year (%y in the locale's %x) is 1969-2068: 2026 either way
    CHECK(parses(loc, s, "%x", got) && got == 2026y / October / 7);
  }
  {
    year_month_day got{};
    CHECK(parses(loc, c_wftime(name, L"%x", t), L"%x", got) && got == 2026y / October / 7);
  }
  // %X: a time of day, read into a duration ([time.parse]/16)
  for (const std::string& s : {std::format(loc, "{:L%X}", tp), c_ftime(name, "%X", t)}) {
    seconds got{};
    CHECK(parses(loc, s, "%X", got) && got == 13h + 4min + 5s);
  }
  // %a %d %b %Y with the C library's names, and the full names
  for (const char* f : {"%a %d %b %Y", "%A %d %B %Y"}) {
    year_month_day got{};
    CHECK(parses(loc, c_ftime(name, f, t), f, got) && got == 2026y / October / 7);
  }
  // wide: the names in any case (towupper of the C library)
  {
    year_month_day got{};
    const std::wstring w = wide_upper(name, c_wftime(name, L"%A %d %B %Y", t));
    CHECK(parses(loc, w, L"%A %d %B %Y", got) && got == 2026y / October / 7);
    weekday wd{};
    CHECK(parses(loc, wide_upper(name, c_wftime(name, L"%a", t)), L"%a", wd) && wd == Wednesday);
  }
  // a weekday that disagrees with the date fails ([time.parse]/17)
  {
    const std::tm thu = tm_of(2026, 10, 8, 0, 0, 0, 4, 280);
    year_month_day got{};
    CHECK(!parses(loc, c_ftime(name, "%a", thu) + " 07 " + c_ftime(name, "%b", t) + " 2026", "%a %d %b %Y", got));
  }
}

int main() {
  const char* de = require_locale("de_DE.UTF-8");
  const char* us = require_locale("en_US.UTF-8");
  check_locale(de);
  check_locale(us);
  for (const char* other : {"fr_FR.UTF-8", "ru_RU.UTF-8", "ja_JP.UTF-8", "de_DE.ISO8859-1"})
    if (c_has_locale(other))
      check_locale(other);

  // %p and %r: the locale's AM/PM strings (en_US has them), in any case, either order
  {
    const std::locale loc(us);
    const std::string am = c_langinfo(us, AM_STR), pm = c_langinfo(us, PM_STR);
    CHECK(!am.empty() && !pm.empty());
    seconds got{};
    CHECK(parses(loc, "01:04 " + pm, "%I:%M %p", got) && got == 13h + 4min);
    got = {};
    CHECK(parses(loc, ascii_upper(pm) + " 01:04", "%p %I:%M", got) && got == 13h + 4min);
    got = {};
    CHECK(parses(loc, "12:30 " + am, "%I:%M %p", got) && got == 30min);
    const std::tm t = tm_of(2026, 10, 7, 13, 4, 5, 3, 279);
    got = {};
    CHECK(parses(loc, c_ftime(us, "%r", t), "%r", got) && got == 13h + 4min + 5s);
    got = {};
    CHECK(parses(loc, std::format(loc, "{:L%r}", 13h + 4min + 5s), "%r", got) && got == 13h + 4min + 5s);
  }

  // The stream's locale decides: a German month name is not a "C" locale name.
  {
    const std::locale loc(de);
    const std::string okt = c_langinfo(de, ABMON_10);
    month m{};
    CHECK(parses(loc, okt + " ", "%b", m) && m == October);
    std::istringstream is(c_langinfo(de, MON_3)); // the classic locale: "März" is no month
    is >> parse("%b", m);
    CHECK(is.fail() || c_langinfo(de, MON_3) == "March");
    // a stream constructed after locale::global(loc) has it ([locale.statics]/1, [ios.base.locales])
    const std::locale old = std::locale::global(loc);
    std::istringstream g(c_langinfo(de, MON_10));
    m = {};
    g >> parse("%B", m);
    CHECK(!g.fail() && m == October);
    std::locale::global(old);
  }
  // %c, %x, %X of the classic locale are unchanged: ISO C's "%a %b %e %H:%M:%S %Y", "%m/%d/%y", "%H:%M:%S"
  {
    sys_seconds got{};
    CHECK(parses(std::locale::classic(), std::string("Wed Oct  7 13:04:05 2026"), "%c", got) &&
          got == sys_days{2026y / October / 7} + 13h + 4min + 5s);
  }
  return 0;
}
