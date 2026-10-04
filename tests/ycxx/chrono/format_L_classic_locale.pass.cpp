// The L option with the classic locale. [time.format]/2: the formatting locale is (2.1) the
// "C" locale if the L option is not present, otherwise (2.2) the locale passed to the
// formatting function, otherwise (2.3) the global locale. [locale.statics]/2: classic()
// returns the "C" locale (also the initial global locale, [locale.cons]/1). So with the
// classic locale passed or global, "{:L<spec>}" and "{:<spec>}" use the same formatting
// locale and give the same result for every conversion specifier of Table 133, including
// the locale-dependent ones (%c %x %X %r %p %a %b %EX %Ex %Ec %OH ...), and both throw
// format_error ([time.format]/3) when the value lacks the information.
// Values chosen where a locale-based path is most likely to diverge from the plain one:
// negative and five-digit years (%c %x %Ex include the year), hh_mm_ss and durations of 24
// hours or more (%X %EX %r include the hour), negative durations ([time.format]/4), and
// subsecond precision (%S's decimal point is "localized according to the locale").
#include <chrono>
#include <format>
#include <locale>
#include <string>
#include <string_view>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

static constexpr const char* specs[] = {
    "%c",  "%Ec", "%x",  "%Ex", "%X",  "%EX", "%r",  "%R",  "%T",  "%p",  "%I",  "%OI", "%H",  "%OH",
    "%M",  "%OM", "%S",  "%OS", "%a",  "%A",  "%b",  "%B",  "%h",  "%d",  "%Od", "%e",  "%Oe", "%m",
    "%Om", "%y",  "%Oy", "%Ey", "%Y",  "%EY", "%C",  "%EC", "%D",  "%F",  "%g",  "%G",  "%j",  "%u",
    "%Ou", "%w",  "%Ow", "%U",  "%OU", "%W",  "%OW", "%V",  "%OV", "%z",  "%Ez", "%Oz", "%Z",  "%Q",
    "%q",  "%n",  "%t",  "%%",  "[%c|%X]"};

template <class T>
std::string fmt(std::string_view spec, const T& v, bool L, int how, bool& threw) {
  std::string f = std::string("{:") + (L ? "L" : "") + std::string(spec) + "}";
  threw = false;
  try {
    if (how == 0)
      return std::vformat(f, std::make_format_args(v));
    return std::vformat(how == 1 ? std::locale::classic() : std::locale("C"), f, std::make_format_args(v));
  } catch (const std::format_error&) {
    threw = true;
    return {};
  }
}

std::wstring widen(const std::string& s) { return std::wstring(s.begin(), s.end()); }

template <class T>
void same(const T& v) {
  for (const char* spec : specs) {
    bool t0;
    const std::string plain = fmt(spec, v, false, 0, t0);
    for (int how = 0; how < 3; ++how) {
      bool t1;
      const std::string loc = fmt(spec, v, true, how, t1);
      if (t1 != t0 || loc != plain)
        dprintf(2, "spec %s (how %d): \"%s\"%s vs L \"%s\"%s\n", spec, how, plain.c_str(), t0 ? " (threw)" : "",
                loc.c_str(), t1 ? " (threw)" : "");
      CHECK(t1 == t0);
      CHECK(loc == plain);
    }
    // wchar_t: the classic locale again (all characters are basic).
    std::wstring wf = L"{:L" + widen(spec) + L"}", wp = L"{:" + widen(spec) + L"}";
    std::wstring a, b;
    bool ta = false, tb = false;
    try { a = std::vformat(wf, std::make_wformat_args(v)); } catch (const std::format_error&) { ta = true; }
    try { b = std::vformat(wp, std::make_wformat_args(v)); } catch (const std::format_error&) { tb = true; }
    CHECK(ta == tb && ta == t0);
    CHECK(a == b && b == widen(plain));
  }
}

int main() {
  CHECK(std::locale() == std::locale::classic());

  same(sys_seconds{sys_days{2021y / January / 3} + 13h + 4min + 5s});
  same(sys_time<milliseconds>{sys_days{1999y / December / 31} + 23h + 59min + 59s + 7ms});
  same(sys_days{-1y / January / 1});           // year -1: %Y "-0001", %y "01", %C "-01"
  same(sys_days{-1976y / June / 15} + 1h);
  same(sys_days{12345y / March / 1} + 23h);    // five-digit year
  same(sys_days{0y / December / 31});
  same(local_time<microseconds>{local_days{1900y / February / 28} + 12h + 1us});
  same(utc_clock::from_sys(sys_days{2016y / December / 31} + 23h + 59min + 59s) + 1s + 250ms); // :60
  same(zoned_time{"Asia/Kolkata", sys_days{2020y / June / 1} + 12h});
  same(zoned_time{"America/St_Johns", sys_days{2020y / June / 1} + 12h});

  same(3723s);
  same(-3723s);
  same(25h + 1min + 2s);                         // a duration of more than a day
  same(-(49h + 30min));
  same(duration<long long, std::milli>{-90'061'001});
  same(duration<double>{1.5});
  same(hh_mm_ss{25h + 1min + 2s + 30ms});       // hours() == 25
  same(hh_mm_ss{-(25h + 1min + 2s + 30ms)});
  same(hh_mm_ss{13h + 0min});

  same(2021y / January / 3);
  same(-5y / July / 4);
  same(year_month_day{2021y / February / 30});   // !ok()
  same(Sunday);
  same(weekday{9});
  same(January);
  same(month{13});
  same(year{-32767});
  same(2024y / February / last);
  same(year_month_weekday{2021y / January / Sunday[1]});
  same(February / 29);
  return 0;
}
