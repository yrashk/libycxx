// {:L} of a chrono value with a named locale, and with a program's facet derived from
// time_put_byname. [time.format]/2: with L the formatting locale is the locale passed to
// format; [time.format]/3, Table 133: %a, %b, %x ... are "the locale's" names and
// representations, written by the locale's time_put ([locale.time.put]); a program's
// time_put_byname-derived facet sees each such conversion and may defer to its base, which
// writes what the C library's strftime does in that locale ([locale.time.put.byname]).
// The decimal numbers (%d %H %Y ...) are the locale's in no locale.
#include <time.h>
#include <chrono>
#include <format>
#include <locale>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

static std::string c_ftime(const char* name, const char* fmt, const std::tm& t) {
  return in_c_locale(name, [&] {
    char b[512];
    const size_t n = strftime(b, sizeof b, fmt, &t);
    return std::string(b, n);
  });
}

// %a is "<weekday>", everything else the named locale's
struct MyTimePut : std::time_put_byname<char> {
  explicit MyTimePut(const char* name) : std::time_put_byname<char>(name) {}
  iter_type do_put(iter_type s, std::ios_base& f, char fill, const std::tm* t, char spec, char mod) const override {
    if (spec == 'a' && mod == 0) {
      for (char c : std::string("<weekday>"))
        *s++ = c;
      return s;
    }
    return std::time_put_byname<char>::do_put(s, f, fill, t, spec, mod);
  }
};

int main() {
  const char* de = require_locale("de_DE.UTF-8");
  std::tm t{};
  t.tm_year = 2026 - 1900;
  t.tm_mon = 9;
  t.tm_mday = 7;
  t.tm_hour = 13;
  t.tm_min = 4;
  t.tm_sec = 5;
  t.tm_wday = 3;
  t.tm_yday = 279;
  const sys_seconds tp = sys_days{2026y / October / 7} + 13h + 4min + 5s;

  // the named locale's own time_put_byname: strftime's text
  const std::locale named(de);
  CHECK(std::format(named, "{:L%a %A %b %B %x %X}", tp) == c_ftime(de, "%a %A %b %B %x %X", t));
  CHECK(std::format(named, "{:L%d.%m.%Y %H:%M}", tp) == "07.10.2026 13:04");
  CHECK(std::format(named, "{:%a %b}", tp) == "Wed Oct"); // no L: the "C" locale

  // a program's facet derived from it
  const std::locale mine(std::locale::classic(), new MyTimePut(de));
  CHECK(std::format(mine, "{:L%a|%A|%b|%x}", tp) == "<weekday>|" + c_ftime(de, "%A|%b|%x", t));
  CHECK(std::format(mine, "{:L}", Wednesday) == "<weekday>"); // [time.cal.wd.nonmembers]/7: {:L%a}
  CHECK(std::format(mine, "{:L}", October) == c_ftime(de, "%b", t));
  CHECK(std::format(mine, "{:%a}", tp) == "Wed");
  return 0;
}
