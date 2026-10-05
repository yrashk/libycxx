// [locale.time.put.byname], [locale.time.get.byname]: the time facets of a named locale.
// time_put writes each conversion as strftime does in that locale ([locale.time.put.virtuals]/1:
// "as if by strftime"); the C library itself is the reference. time_get reads the locale's
// weekday and month names, full and abbreviated (nl_langinfo DAY_n, ABDAY_n, MON_n, ABMON_n),
// orders dates as the locale's %x does ([locale.time.get.virtuals]/1, date_order), and reads back
// what time_put writes for %x, %X and %c ([locale.time.get.virtuals]/4, 11-13).
#include <langinfo.h>
#include <time.h>
#include <wchar.h>
#include <algorithm>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

static std::tm sample() {
  std::tm t{};
  t.tm_year = 2009 - 1900;
  t.tm_mon = 5;
  t.tm_mday = 10;
  t.tm_wday = 3;
  t.tm_yday = 160;
  t.tm_hour = 13;
  t.tm_min = 5;
  t.tm_sec = 9;
  return t;
}

template <class charT>
static std::basic_string<charT> put(const std::locale& l, const std::tm& t, const char* fmt) {
  std::basic_ostringstream<charT> os;
  os.imbue(l);
  std::basic_string<charT> f;
  for (const char* p = fmt; *p; ++p)
    f.push_back(static_cast<charT>(*p));
  std::use_facet<std::time_put<charT>>(l).put(std::ostreambuf_iterator<charT>(os), os, charT(' '), &t, f.data(),
                                               f.data() + f.size());
  return os.str();
}

template <class charT>
static std::tm get(const std::locale& l, const std::basic_string<charT>& s, char what,
                   std::ios_base::iostate& err) {
  std::basic_istringstream<charT> is(s);
  is.imbue(l);
  const auto& tg = std::use_facet<std::time_get<charT>>(l);
  std::tm t{};
  t.tm_year = -1;
  t.tm_mon = -1;
  t.tm_mday = -1;
  t.tm_wday = -1;
  err = std::ios_base::goodbit;
  std::istreambuf_iterator<charT> b(is), e;
  switch (what) {
  case 'a':
    tg.get_weekday(b, e, is, err, &t);
    break;
  case 'b':
    tg.get_monthname(b, e, is, err, &t);
    break;
  case 'x':
    tg.get_date(b, e, is, err, &t);
    break;
  default:
    tg.get(b, e, is, err, &t, what, 0);
  }
  return t;
}

static void check(const char* name) {
  const std::locale l(name);
  const std::tm t = sample();
  for (const char* f : {"%a", "%A", "%b", "%B", "%c", "%x", "%X", "%p", "%r", "%Ex", "%Od", "%Y-%m-%d %H:%M:%S",
                        "[%e|%j|%u|%w|%U|%W|%V|%G|%y|%C]"}) {
    const std::string want = in_c_locale(name, [&] {
      char buf[256];
      return std::string(buf, strftime(buf, sizeof buf, f, &t));
    });
    CHECK(put<char>(l, t, f) == want);
    const std::wstring wwant = in_c_locale(name, [&] {
      wchar_t buf[256];
      wchar_t wf[64];
      std::size_t k = 0;
      for (const char* p = f; *p; ++p)
        wf[k++] = static_cast<wchar_t>(*p);
      wf[k] = 0;
      return std::wstring(buf, wcsftime(buf, 256, wf, &t));
    });
    CHECK(put<wchar_t>(l, t, f) == wwant);
  }

  // the names: each one of the locale's reads back
  static const nl_item days[] = {DAY_1, DAY_2, DAY_3, DAY_4, DAY_5, DAY_6, DAY_7};
  static const nl_item abdays[] = {ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7};
  static const nl_item mons[] = {MON_1, MON_2, MON_3, MON_4, MON_5, MON_6, MON_7, MON_8, MON_9, MON_10, MON_11, MON_12};
  static const nl_item abmons[] = {ABMON_1, ABMON_2, ABMON_3, ABMON_4,  ABMON_5,  ABMON_6,
                                   ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12};
  std::ios_base::iostate err;
  for (int i = 0; i < 7; ++i)
    for (const nl_item* items : {days, abdays}) {
      const std::string s = in_c_locale(name, [&] { return std::string(nl_langinfo(items[i])); });
      CHECK(get<char>(l, s, 'a', err).tm_wday == i);
      CHECK(!(err & std::ios_base::failbit));
    }
  for (int i = 0; i < 12; ++i)
    for (const nl_item* items : {mons, abmons}) {
      const std::string s = in_c_locale(name, [&] { return std::string(nl_langinfo(items[i])); });
      CHECK(get<char>(l, s, 'b', err).tm_mon == i);
      CHECK(!(err & std::ios_base::failbit));
    }
  // wide: the names written by time_put read back
  for (const char* f : {"%A", "%a"})
    CHECK(get<wchar_t>(l, put<wchar_t>(l, t, f), 'a', err).tm_wday == 3 && !(err & std::ios_base::failbit));
  for (const char* f : {"%B", "%b"})
    CHECK(get<wchar_t>(l, put<wchar_t>(l, t, f), 'b', err).tm_mon == 5 && !(err & std::ios_base::failbit));

  // date_order from %x, and %x / %X / %c read back
  const std::string dfmt = in_c_locale(name, [] { return std::string(nl_langinfo(D_FMT)); });
  const auto pos = [&](const char* alts) {
    std::size_t best = std::string::npos;
    for (const char* a = alts; *a; ++a) {
      const char conv[3] = {'%', *a, 0};
      best = std::min(best, dfmt.find(conv));
    }
    return best;
  };
  const std::size_t d = pos("de"), m = pos("mbBh"), y = pos("yY");
  std::time_base::dateorder want = std::time_base::no_order;
  if (d < m && m < y)
    want = std::time_base::dmy;
  else if (m < d && d < y)
    want = std::time_base::mdy;
  else if (y < m && m < d)
    want = std::time_base::ymd;
  else if (y < d && d < m)
    want = std::time_base::ydm;
  CHECK(std::use_facet<std::time_get<char>>(l).date_order() == want);
  const std::tm dx = get<char>(l, put<char>(l, t, "%x"), 'x', err);
  CHECK(!(err & std::ios_base::failbit) && dx.tm_mday == 10 && dx.tm_mon == 5 && dx.tm_year == 109);
  const std::tm wdx = get<wchar_t>(l, put<wchar_t>(l, t, "%x"), 'x', err);
  CHECK(!(err & std::ios_base::failbit) && wdx.tm_mday == 10 && wdx.tm_mon == 5 && wdx.tm_year == 109);
  const std::tm tx = get<char>(l, put<char>(l, t, "%X"), 'X', err);
  CHECK(!(err & std::ios_base::failbit) && tx.tm_hour == 13 && tx.tm_min == 5 && tx.tm_sec == 9);
}

int main() {
  check(require_locale("de_DE.UTF-8"));
  check(require_locale("fr_FR.ISO8859-15"));
  check(require_locale("en_US.UTF-8"));
  check(require_locale("ja_JP.UTF-8"));
}
