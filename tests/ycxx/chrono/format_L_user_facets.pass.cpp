// The L option formats through the facets of the formatting locale, a program's own included.
// [time.format]/2: with L the formatting locale is the locale passed to the formatting function
// (else the global locale); without L it is the "C" locale.
// [time.format]/3 and Table 133: the locale-dependent conversions are "the locale's abbreviated
// weekday name", "... date and time representation", "the locale's alternative representation"
// (every E and O form) and so on; the locale's time_put ([locale.time.put]) writes them, so a
// program's time_put sees each of them as one put(..., spec, modifier) call. The specifiers
// described as decimal numbers (%d %H %Y %F %T ...) do not depend on the locale.
// [time.format]/7: without chrono-specs the value is formatted "as if by streaming it to
// basic_ostringstream<charT> os with the formatting locale imbued"; [time.duration.io]/1 inserts
// a duration as `s << d.count()`, so the count goes through the locale's num_put, with the
// overload the rep's operator<< calls ([ostream.inserters.arithmetic]/1: int as long, float as
// double). %Q is the count "as if extracted via .count()": not the locale's.
#include <chrono>
#include <format>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

// num_put that marks which overload it was called with.
template <class C>
struct TagNumPut : std::num_put<C> {
  using iter_type = typename std::num_put<C>::iter_type;
  static iter_type tag(iter_type s, const char* t) {
    for (; *t; ++t)
      *s++ = C(*t);
    return s;
  }
  iter_type do_put(iter_type s, std::ios_base&, C, long v) const override {
    return tag(s, v == 12 ? "<long 12>" : "<long>");
  }
  iter_type do_put(iter_type s, std::ios_base&, C, unsigned long) const override { return tag(s, "<ulong>"); }
  iter_type do_put(iter_type s, std::ios_base&, C, long long v) const override {
    return tag(s, v == 12345 ? "<llong 12345>" : "<llong>");
  }
  iter_type do_put(iter_type s, std::ios_base&, C, unsigned long long) const override { return tag(s, "<ullong>"); }
  iter_type do_put(iter_type s, std::ios_base& f, C, double) const override {
    return tag(s, f.precision() == 3 ? "<double p3>" : f.precision() == 6 ? "<double p6>" : "<double>");
  }
  iter_type do_put(iter_type s, std::ios_base&, C, long double) const override { return tag(s, "<ldouble>"); }
};

// time_put that writes "<spec>" or "<modifier spec>" and the year it was given for %c.
template <class C>
struct TagTimePut : std::time_put<C> {
  using iter_type = typename std::time_put<C>::iter_type;
  iter_type do_put(iter_type s, std::ios_base&, C, const std::tm* t, char spec, char mod) const override {
    *s++ = C('<');
    if (mod != 0)
      *s++ = C(mod);
    *s++ = C(spec);
    if (spec == 'c') {
      const std::string y = std::to_string(t->tm_year + 1900) + "-" + std::to_string(t->tm_mon + 1) + "-" +
                            std::to_string(t->tm_mday) + "/" + std::to_string(t->tm_wday);
      for (char ch : y)
        *s++ = C(ch);
    }
    *s++ = C('>');
    return s;
  }
};

template <class D>
std::string streamed(const std::locale& loc, D d) {
  std::ostringstream os;
  os.imbue(loc);
  os << d;
  return os.str();
}

int main() {
  // ---- num_put: the count of a duration without chrono-specs ----
  const std::locale np(std::locale(std::locale::classic(), new TagNumPut<char>), new TagNumPut<wchar_t>);
  CHECK(std::format(np, "{:L}", 12345ms) == "<llong 12345>ms");
  CHECK(streamed(np, 12345ms) == "<llong 12345>ms");
  CHECK(std::format(np, "{:L}", duration<int>(12)) == "<long 12>s"); // int: put(long)
  CHECK(streamed(np, duration<int>(12)) == "<long 12>s");
  CHECK(std::format(np, "{:L}", duration<short, std::milli>(12)) == "<long 12>ms");
  CHECK(std::format(np, "{:L}", duration<unsigned>(12)) == "<ulong>s");
  CHECK(std::format(np, "{:L}", duration<unsigned long long>(12)) == "<ullong>s");
  CHECK(std::format(np, "{:L}", duration<float>(1.5f)) == "<double p6>s"); // float: put(double)
  CHECK(std::format(np, "{:L}", duration<double>(1.5)) == "<double p6>s");
  CHECK(std::format(np, "{:.3L}", duration<double>(1.5)) == "<double p3>s");
  CHECK(std::format(np, "{:L}", duration<long double>(1.5)) == "<ldouble>s");
  CHECK(streamed(np, duration<long double>(1.5)) == "<ldouble>s");
  CHECK(std::format(np, "{:>17L}", 12345ms) == "  <llong 12345>ms"); // padding applies to the whole
  CHECK(std::format(np, L"{:L}", 12345ms) == L"<llong 12345>ms");
  CHECK(std::format(np, L"{:L}", duration<int>(12)) == L"<long 12>s");
  // Without L: the "C" locale.
  CHECK(std::format(np, "{}", 12345ms) == "12345ms");
  CHECK(std::format(np, "{}", duration<double>(1.5)) == "1.5s");
  // %Q is the count, not the locale's.
  CHECK(std::format(np, "{:L%Q%q}", 12345ms) == "12345ms");
  // The global locale with L and no locale argument.
  {
    const std::locale old = std::locale::global(np);
    CHECK(std::format("{:L}", 12345ms) == "<llong 12345>ms");
    CHECK(std::format("{}", 12345ms) == "12345ms");
    std::locale::global(old);
  }

  // ---- time_put: every locale-dependent conversion, and only those ----
  const std::locale tp(std::locale(std::locale::classic(), new TagTimePut<char>), new TagTimePut<wchar_t>);
  const sys_seconds t = sys_days{2026y / October / 7} + 13h + 4min + 5s; // a Wednesday
  CHECK(std::format(tp, "{:L%a %A %b %B %h %p %r %x %X}", t) == "<a> <A> <b> <B> <h> <p> <r> <x> <X>");
  CHECK(std::format(tp, "{:L%c}", t) == "<c2026-10-7/3>");
  CHECK(std::format(tp, "{:L%Ec %EC %Ex %EX %Ey %EY}", t) == "<Ec2026-10-7/3> <EC> <Ex> <EX> <Ey> <EY>");
  CHECK(std::format(tp, "{:L%Od %Oe %OH %OI %Om %OM %OS %Ou %OU %OV %Ow %OW %Oy}", t) ==
        "<Od> <Oe> <OH> <OI> <Om> <OM> <OS> <Ou> <OU> <OV> <Ow> <OW> <Oy>");
  // decimal numbers and ISO forms do not depend on the locale
  CHECK(std::format(tp, "{:L%d %e %H %I %m %M %S %u %w %y %Y %C %j %F %T %R %D %G %g %V %U %W}", t) ==
        "07  7 13 01 10 04 05 3 3 26 2026 20 280 2026-10-07 13:04:05 13:04 10/07/26 2026 26 41 40 40");
  CHECK(std::format(tp, "{:L%z %Ez %Oz}", t) == "+0000 +00:00 +00:00");
  CHECK(std::format(tp, "{:L%Z}", t) == "UTC");
  // without L: the "C" locale, no call
  CHECK(std::format(tp, "{:%a %b %c %p}", t) == "Wed Oct Wed Oct  7 13:04:05 2026 PM");
  CHECK(std::format(tp, "{:%Od %EY}", t) == "07 2026");
  // the default representations that use a name: [time.cal.wd.nonmembers]/7, [time.cal.month.nonmembers]/7
  CHECK(std::format(tp, "{:L}", Wednesday) == "<a>");
  CHECK(std::format(tp, "{:L}", October) == "<b>");
  CHECK(std::format(tp, "{}", October) == "Oct");
  CHECK(std::format(tp, "{:L}", t) == "2026-10-07 13:04:05"); // %F %T: no name
  CHECK(std::format(tp, L"{:L%a %Od %c}", t) == L"<a> <Od> <c2026-10-7/3>");
  // durations: a time of day
  CHECK(std::format(tp, "{:L%p %OH}", 13h + 4min) == "<p> <OH>");
  // both facets at once
  const std::locale both(np, new TagTimePut<char>);
  CHECK(std::format(both, "{:L} {:L%a}", 12345ms, t) == "<llong 12345>ms <a>");
  return 0;
}
