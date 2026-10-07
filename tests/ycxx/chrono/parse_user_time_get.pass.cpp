// chrono::parse with a program's own time_get facet in the stream's locale.
// [time.parse] Table 134 reads "the locale's" names, AM/PM designations, date and time
// representations and alternative representations; the locale's time_get<charT,
// istreambuf_iterator<charT, traits>> is the facet that reads them ([locale.time.get]: do_get
// "interprets ... as the strptime conversion"), so a program's facet installed in the stream's
// locale decides what they accept: each locale-dependent flag is one get(..., spec, modifier)
// call of that facet. Flags with no locale in their wording (%d %Y %H ...) do not call it.
// A locale whose time_get is the classic one keeps the "C" locale's conventions.
#include <chrono>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

static int calls = 0;      // get calls of the facet
static std::string trace;  // their conversions

// Months "M01".."M12", weekdays "D0".."D6", "soir"/"matin" for PM/AM, %c as
// "Y<year>|<mon>|<mday>|<hour>|<min>|<sec>", %Od as a Roman numeral (i-xxxi), %EY as "AD<year>".
struct MyTimeGet : std::time_get<char> {
  static int number(iter_type& s, iter_type end, int max_digits) {
    int v = 0, n = 0;
    for (; n < max_digits && s != end && *s >= '0' && *s <= '9'; ++n, ++s)
      v = v * 10 + (*s - '0');
    return n == 0 ? -1 : v;
  }
  static bool lit(iter_type& s, iter_type end, const char* text) {
    for (; *text; ++text, ++s)
      if (s == end || *s != *text)
        return false;
    return true;
  }
  iter_type do_get(iter_type s, iter_type end, std::ios_base& f, std::ios_base::iostate& err, std::tm* t, char spec,
                   char mod) const override {
    ++calls;
    trace += mod ? std::string{'%', mod, spec} : std::string{'%', spec};
    err = std::ios_base::goodbit;
    int v;
    switch (spec) {
    case 'b':
    case 'B':
    case 'h':
      if (!lit(s, end, "M") || (v = number(s, end, 2)) < 1 || v > 12)
        err = std::ios_base::failbit;
      else
        t->tm_mon = v - 1;
      return s;
    case 'a':
    case 'A':
      if (!lit(s, end, "D") || (v = number(s, end, 1)) < 0 || v > 6)
        err = std::ios_base::failbit;
      else
        t->tm_wday = v;
      return s;
    case 'p':
      if (s != end && *s == 's' && lit(s, end, "soir"))
        t->tm_hour = t->tm_hour % 12 + 12;
      else if (s != end && *s == 'm' && lit(s, end, "matin"))
        t->tm_hour = t->tm_hour % 12;
      else
        err = std::ios_base::failbit;
      return s;
    case 'c': {
      int y, mo, d, h, mi, sec;
      if (!lit(s, end, "Y") || (y = number(s, end, 4)) < 0 || !lit(s, end, "|") || (mo = number(s, end, 2)) < 0 ||
          !lit(s, end, "|") || (d = number(s, end, 2)) < 0 || !lit(s, end, "|") || (h = number(s, end, 2)) < 0 ||
          !lit(s, end, "|") || (mi = number(s, end, 2)) < 0 || !lit(s, end, "|") || (sec = number(s, end, 2)) < 0) {
        err = std::ios_base::failbit;
        return s;
      }
      t->tm_year = y - 1900, t->tm_mon = mo - 1, t->tm_mday = d, t->tm_hour = h, t->tm_min = mi, t->tm_sec = sec;
      return s;
    }
    case 'd':
      if (mod == 'O') {
        static const char* const roman[] = {"xxxi", "xxx", "xxix", "xxviii", "xxvii", "xxvi", "xxv", "xxiv", "xxiii",
                                            "xxii", "xxi", "xx", "xix", "xviii", "xvii", "xvi", "xv", "xiv", "xiii",
                                            "xii", "xi", "x", "ix", "viii", "vii", "vi", "v", "iv", "iii", "ii", "i"};
        std::string word;
        while (s != end && (*s == 'i' || *s == 'v' || *s == 'x'))
          word += *s++;
        for (int k = 0; k < 31; ++k)
          if (word == roman[k]) {
            t->tm_mday = 31 - k;
            return s;
          }
        err = std::ios_base::failbit;
        return s;
      }
      break;
    case 'Y':
      if (mod == 'E') {
        if (!lit(s, end, "AD") || (v = number(s, end, 4)) < 0)
          err = std::ios_base::failbit;
        else
          t->tm_year = v - 1900;
        return s;
      }
      break;
    }
    return std::time_get<char>::do_get(s, end, f, err, t, spec, mod);
  }
};

template <class T>
static bool parses(const std::locale& loc, const std::string& in, const char* fmt, T& out) {
  std::istringstream is(in);
  is.imbue(loc);
  is >> parse(fmt, out);
  return !is.fail();
}

int main() {
  const std::locale loc(std::locale::classic(), new MyTimeGet);

  month m{};
  CHECK(parses(loc, "M10", "%b", m) && m == October);
  CHECK(trace == "%b");
  CHECK(parses(loc, "M03", "%B", m) && m == March);
  CHECK(parses(loc, "M12", "%h", m) && m == December);
  CHECK(!parses(loc, "Oct", "%b", m)); // the facet's names, not the "C" locale's
  CHECK(!parses(loc, "M13", "%b", m));

  weekday wd{};
  CHECK(parses(loc, "D3", "%a", wd) && wd == Wednesday);
  CHECK(parses(loc, "D0", "%A", wd) && wd == Sunday);

  // %p with %I, in either order
  seconds tod{};
  CHECK(parses(loc, "01:04 soir", "%I:%M %p", tod) && tod == 13h + 4min);
  CHECK(parses(loc, "soir 01:04", "%p %I:%M", tod) && tod == 13h + 4min);
  CHECK(parses(loc, "12:00 matin", "%I:%M %p", tod) && tod == 0h);
  CHECK(parses(loc, "12:00 soir", "%I:%M %p", tod) && tod == 12h);

  // %c: one call; the fields it set make the value
  sys_seconds tp{};
  trace.clear();
  CHECK(parses(loc, "Y2026|10|07|13|04|05", "%c", tp) && tp == sys_days{2026y / October / 7} + 13h + 4min + 5s);
  CHECK(trace == "%c");
  CHECK(!parses(loc, "Wed Oct  7 13:04:05 2026", "%c", tp));
  // the fields of %c must agree with the others given ([time.parse]/17)
  CHECK(parses(loc, "D3 Y2026|10|07|13|04|05", "%a %c", tp));
  CHECK(!parses(loc, "D4 Y2026|10|07|13|04|05", "%a %c", tp));

  // the E and O forms the facet reads
  year_month_day ymd{};
  CHECK(parses(loc, "vii M10 AD2026", "%Od %b %EY", ymd) && ymd == 2026y / October / 7);
  trace.clear();
  calls = 0;
  // flags that do not depend on the locale do not call it
  CHECK(parses(loc, "2026-10-07 13:04:05", "%Y-%m-%d %H:%M:%S", tp));
  CHECK(calls == 0);
  CHECK(parses(loc, "2026-10-07", "%F", ymd) && calls == 0);
  // a duration's %p
  CHECK(parses(loc, "11 soir", "%I %p", tod) && tod == 23h);

  // wchar_t streams have no such facet in this locale: their "C" conventions
  {
    std::wistringstream is(L"Oct 2026");
    is.imbue(loc);
    year_month ym{};
    is >> parse(L"%b %Y", ym);
    CHECK(!is.fail() && ym == 2026y / October);
  }
  // a locale with the classic time_get: the "C" names, no facet call
  {
    calls = 0;
    const std::locale classic_tg(loc, &std::use_facet<std::time_get<char>>(std::locale::classic()));
    CHECK(parses(classic_tg, "Oct", "%b", m) && m == October);
    CHECK(!parses(classic_tg, "M10", "%b", m));
    CHECK(calls == 0);
  }
  return 0;
}
