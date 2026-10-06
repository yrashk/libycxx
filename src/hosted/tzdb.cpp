// libycxx hosted runtime: the time zone database of <chrono> ([time.zone.db]) on the system's
// compiled zoneinfo directory ($TZDIR, else /usr/share/zoneinfo).
//
// Like filesystem.cpp, this file reads files directly (C stdio, POSIX readlink/opendir) instead
// of going through the PAL: the data format is the zoneinfo directory's, and a port without it
// replaces this file (DECISIONS §14).
//
// - Names: zones ("Z" lines) and links ("L target name") from tzdata.zi; without tzdata.zi, every
//   TZif file of the directory tree is a zone and every symbolic link to one a link.
// - Version: +VERSION, else the "# version" line of tzdata.zi, else "unknown".
// - Leap seconds: the leapseconds file, else leap-seconds.list, else the 27 insertions of
//   1972-2016 (IERS).
// - A zone's transitions are read from its TZif file (versions 1-4; the 64-bit block when there
//   is one) on the first query, under the zone's once_flag. Times after the last transition come
//   from the file's POSIX TZ footer. Consecutive transitions to the same offset, save and
//   abbreviation are merged, so a sys_info covers the whole period its values are in effect.
//   TZif records only whether a type is daylight saving time, so `save` is the difference to
//   the nearest standard-time offset (one hour when that difference is zero).
// - reload_tzdb() reloads when the directory's version differs from front()'s and pushes the new
//   database onto the list under a lock; front() and iteration read the list with acquire loads.
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <new>
#include <string>
#include <string_view>
#include <vector>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace __chr = std::chrono;
using std::int32_t;
using std::int64_t;
using std::uint32_t;
using std::uint64_t;
using std::string;
using std::string_view;
using std::vector;

namespace {

constexpr int64_t min_time = std::numeric_limits<int64_t>::min();
constexpr int64_t max_time = std::numeric_limits<int64_t>::max();

int64_t sat_add(int64_t a, int64_t b) noexcept {
  int64_t r;
  if (__builtin_add_overflow(a, b, &r))
    return b > 0 ? max_time : min_time;
  return r;
}

string zoneinfo_dir() {
  const char* d = ::getenv("TZDIR");
  string s = d != nullptr && *d != '\0' ? d : "/usr/share/zoneinfo";
  while (s.size() > 1 && s.back() == '/')
    s.pop_back();
  return s;
}

// An open file, closed however its reader ends (an allocation failure throws).
struct __file {
  FILE* __f;
  explicit __file(const char* path) noexcept : __f(::fopen(path, "rb")) {}
  __file(const __file&) = delete;
  __file& operator=(const __file&) = delete;
  ~__file() {
    if (__f != nullptr)
      ::fclose(__f);
  }
};

bool read_file(const string& path, string& out) {
  __file in(path.c_str());
  if (in.__f == nullptr)
    return false;
  out.clear();
  char __buf[8192];
  size_t n;
  while ((n = ::fread(__buf, 1, sizeof __buf, in.__f)) != 0)
    out.append(__buf, n);
  return ::ferror(in.__f) == 0;
}

// The lines of `__text`, each without its line terminator.
template <class _Fp>
void for_each_line(string_view __text, _Fp&& __f) {
  while (!__text.empty()) {
    size_t e = __text.find('\n');
    string_view line = __text.substr(0, e);
    if (!line.empty() && line.back() == '\r')
      line.remove_suffix(1);
    __f(line);
    if (e == string_view::npos)
      break;
    __text.remove_prefix(e + 1);
  }
}

// The whitespace-separated fields of a line.
vector<string_view> fields(string_view line) {
  vector<string_view> r;
  size_t i = 0;
  while (i < line.size()) {
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
      ++i;
    size_t __j = i;
    while (__j < line.size() && line[__j] != ' ' && line[__j] != '\t')
      ++__j;
    if (__j > i)
      r.push_back(line.substr(i, __j - i));
    i = __j;
  }
  return r;
}

string __trim(string_view s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\n' || s.front() == '\r'))
    s.remove_prefix(1);
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\n' || s.back() == '\r'))
    s.remove_suffix(1);
  return string(s);
}

int64_t __days_from_civil(int64_t y, unsigned m, unsigned d) {
  return __ycxx::__detail::__days_from_civil(static_cast<int>(y), m, d);
}

// ---- POSIX TZ strings (the TZif footer; RFC 8536 section 3.3) ---------------------------------

struct posix_date {
  char kind = 0; // 'J' (1-365, no leap day), 'n' (0-365), 'M' (month.week.day)
  int a = 0, b = 0, c = 0;
  int64_t time = 7200; // seconds after local midnight, may be negative or beyond 24h
};
struct posix_rule {
  string std_abbrev, dst_abbrev;
  int64_t std_off = 0, dst_off = 0; // UTC offsets (east positive)
  bool has_dst = false;
  posix_date start, end;
};

struct cursor {
  string_view s;
  size_t i = 0;
  bool done() const { return i >= s.size(); }
  char peek() const { return done() ? '\0' : s[i]; }
};

bool parse_number(cursor& c, int& __v) {
  if (c.done() || c.peek() < '0' || c.peek() > '9')
    return false;
  __v = 0;
  while (!c.done() && c.peek() >= '0' && c.peek() <= '9') {
    __v = __v * 10 + (c.peek() - '0');
    if (__v > 100000)
      return false;
    ++c.i;
  }
  return true;
}

// [+|-]hh[:mm[:ss]] as seconds.
bool parse_hms(cursor& c, int64_t& out) {
  int sign = 1;
  if (c.peek() == '+' || c.peek() == '-')
    sign = c.s[c.i++] == '-' ? -1 : 1;
  int h = 0, m = 0, s = 0;
  if (!parse_number(c, h))
    return false;
  if (c.peek() == ':') {
    ++c.i;
    if (!parse_number(c, m))
      return false;
    if (c.peek() == ':') {
      ++c.i;
      if (!parse_number(c, s))
        return false;
    }
  }
  out = sign * (h * 3600LL + m * 60LL + s);
  return true;
}

bool parse_abbrev(cursor& c, string& out) {
  if (c.peek() == '<') {
    const size_t e = c.s.find('>', c.i);
    if (e == string_view::npos)
      return false;
    out.assign(c.s.substr(c.i + 1, e - c.i - 1));
    c.i = e + 1;
    return !out.empty();
  }
  const size_t b = c.i;
  while (!c.done() && ((c.peek() >= 'A' && c.peek() <= 'Z') || (c.peek() >= 'a' && c.peek() <= 'z')))
    ++c.i;
  out.assign(c.s.substr(b, c.i - b));
  return out.size() >= 3;
}

bool parse_date(cursor& c, posix_date& d) {
  if (c.peek() == 'J') {
    ++c.i;
    d.kind = 'J';
    if (!parse_number(c, d.a) || d.a < 1 || d.a > 365)
      return false;
  } else if (c.peek() == 'M') {
    ++c.i;
    d.kind = 'M';
    if (!parse_number(c, d.a) || c.peek() != '.')
      return false;
    ++c.i;
    if (!parse_number(c, d.b) || c.peek() != '.')
      return false;
    ++c.i;
    if (!parse_number(c, d.c))
      return false;
    if (d.a < 1 || d.a > 12 || d.b < 1 || d.b > 5 || d.c > 6)
      return false;
  } else {
    d.kind = 'n';
    if (!parse_number(c, d.a) || d.a > 365)
      return false;
  }
  d.time = 7200;
  if (c.peek() == '/') {
    ++c.i;
    if (!parse_hms(c, d.time))
      return false;
  }
  return true;
}

bool parse_posix_tz(string_view s, posix_rule& r) {
  cursor c{s};
  int64_t __off;
  if (!parse_abbrev(c, r.std_abbrev) || !parse_hms(c, __off))
    return false;
  r.std_off = -__off;
  if (c.done())
    return true;
  if (!parse_abbrev(c, r.dst_abbrev))
    return false;
  r.has_dst = true;
  r.dst_off = r.std_off + 3600;
  if (c.peek() != ',' && !c.done()) {
    if (!parse_hms(c, __off))
      return false;
    r.dst_off = -__off;
  }
  if (c.done()) { // no rule: the POSIX default (the United States rules)
    r.start = {'M', 3, 2, 0, 7200};
    r.end = {'M', 11, 1, 0, 7200};
    return true;
  }
  if (c.peek() != ',')
    return false;
  ++c.i;
  if (!parse_date(c, r.start) || c.peek() != ',')
    return false;
  ++c.i;
  return parse_date(c, r.end) && c.done();
}

// The local time (seconds since the epoch) of a rule date in year y.
int64_t rule_local_time(const posix_date& d, int64_t y) {
  int64_t day;
  if (d.kind == 'J') {
    day = __days_from_civil(y, 1, 1) + d.a - 1;
    if (__ycxx::__detail::__is_leap_year(static_cast<int>(y)) && d.a >= 60)
      ++day;
  } else if (d.kind == 'n') {
    day = __days_from_civil(y, 1, 1) + d.a;
  } else {
    const int64_t first = __days_from_civil(y, static_cast<unsigned>(d.a), 1);
    const unsigned __wd1 = __ycxx::__detail::__weekday_from_days(first);
    int64_t day_of = first + (d.c - static_cast<int>(__wd1) + 7) % 7 + (d.b - 1) * 7;
    const int64_t last = __days_from_civil(y, static_cast<unsigned>(d.a), 1) +
                         __ycxx::__detail::__last_day_of(static_cast<int>(y), static_cast<unsigned>(d.a)) - 1;
    while (day_of > last)
      day_of -= 7;
    day = day_of;
  }
  return day * 86400 + d.time;
}

// ---- the zone data ------------------------------------------------------------------------------

struct __period {
  int64_t begin;
  int32_t offset;  // seconds
  int32_t save;    // minutes
  uint32_t abbrev; // index into tz_data::abbrevs
};

struct info {
  int64_t begin, end;
  int64_t offset;
  int64_t save;
  string_view abbrev;
};

} // namespace

struct __ycxx::__detail::__tz_data {
  string name;
  std::once_flag once;
  bool ok = false;
  vector<__period> periods; // sorted by begin; periods[0].begin == min_time
  vector<string> abbrevs;
  bool has_footer = false;
  posix_rule rule;
  int64_t footer_from = min_time; // the last transition: the footer governs times at or after it
  uint32_t std_abbrev = 0, dst_abbrev = 0;

  explicit __tz_data(string n) : name(static_cast<string&&>(n)) {}

  uint32_t abbrev_index(string_view a) {
    for (size_t i = 0; i < abbrevs.size(); ++i)
      if (abbrevs[i] == a)
        return static_cast<uint32_t>(i);
    abbrevs.emplace_back(a);
    return static_cast<uint32_t>(abbrevs.size() - 1);
  }
  void load();
  bool load_tzif(const string& __bytes);
  info rule_info(int64_t t) const;
  info sys_info(int64_t t) const;
};

void __ycxx::__detail::__tz_data_deleter::operator()(__tz_data* p) const noexcept { delete p; }

namespace {

uint32_t be32(const unsigned char* p) {
  return static_cast<uint32_t>(p[0]) << 24 | static_cast<uint32_t>(p[1]) << 16 | static_cast<uint32_t>(p[2]) << 8 |
         static_cast<uint32_t>(p[3]);
}
int64_t be64(const unsigned char* p) {
  return static_cast<int64_t>(static_cast<uint64_t>(be32(p)) << 32 | be32(p + 4));
}

} // namespace

bool __ycxx::__detail::__tz_data::load_tzif(const string& __bytes) {
  const unsigned char* p = reinterpret_cast<const unsigned char*>(__bytes.data());
  const unsigned char* const e = p + __bytes.size();
  struct __header {
    char version;
    uint32_t isutcnt, isstdcnt, leapcnt, timecnt, typecnt, charcnt;
  };
  auto read_header = [&](__header& h) {
    if (e - p < 44 || ::memcmp(p, "TZif", 4) != 0)
      return false;
    h.version = static_cast<char>(p[4]);
    h.isutcnt = be32(p + 20);
    h.isstdcnt = be32(p + 24);
    h.leapcnt = be32(p + 28);
    h.timecnt = be32(p + 32);
    h.typecnt = be32(p + 36);
    h.charcnt = be32(p + 40);
    p += 44;
    return h.typecnt != 0 && h.typecnt <= 256 && h.timecnt <= 1000000 && h.charcnt <= 65536 && h.leapcnt <= 100000;
  };
  auto block_size = [](const __header& h, size_t tsize) {
    return h.timecnt * tsize + h.timecnt + h.typecnt * 6 + h.charcnt + h.leapcnt * (tsize + 4) + h.isstdcnt +
           h.isutcnt;
  };
  __header h;
  if (!read_header(h))
    return false;
  size_t tsize = 4;
  if (h.version >= '2') { // skip the 32-bit block, use the 64-bit one
    if (static_cast<size_t>(e - p) < block_size(h, 4))
      return false;
    p += block_size(h, 4);
    if (!read_header(h))
      return false;
    tsize = 8;
  }
  if (static_cast<size_t>(e - p) < block_size(h, tsize))
    return false;
  const unsigned char* times = p;
  const unsigned char* __idx = times + h.timecnt * tsize;
  const unsigned char* __types = __idx + h.timecnt;
  const unsigned char* __chars = __types + h.typecnt * 6;
  struct ttype {
    int32_t utoff;
    bool isdst;
    uint32_t abbrev;
  };
  vector<ttype> tt(h.typecnt);
  for (uint32_t i = 0; i < h.typecnt; ++i) {
    const unsigned char* __q = __types + i * 6;
    tt[i].utoff = static_cast<int32_t>(be32(__q));
    tt[i].isdst = __q[4] != 0;
    const unsigned __di = __q[5];
    string_view a;
    if (__di < h.charcnt) {
      const char* s = reinterpret_cast<const char*>(__chars) + __di;
      a = string_view(s, ::strnlen(s, h.charcnt - __di));
    }
    tt[i].abbrev = abbrev_index(a);
  }
  p = __chars + h.charcnt + h.leapcnt * (tsize + 4) + h.isstdcnt + h.isutcnt;

  // Transitions: before the first one, type 0 applies.
  struct trans {
    int64_t at;
    uint32_t type;
  };
  vector<trans> __tr;
  __tr.push_back({min_time, 0});
  for (uint32_t i = 0; i < h.timecnt; ++i) {
    const int64_t at = tsize == 8 ? be64(times + i * 8) : static_cast<int32_t>(be32(times + i * 4));
    const uint32_t ty = __idx[i];
    if (ty >= h.typecnt)
      return false;
    if (at <= __tr.back().at)
      continue;
    __tr.push_back({at, ty});
  }
  footer_from = __tr.size() > 1 ? __tr.back().at : min_time;

  // save: the difference to the nearest standard-time offset, earlier transitions first.
  periods.clear();
  for (size_t i = 0; i < __tr.size(); ++i) {
    const ttype& t = tt[__tr[i].type];
    int32_t save = 0;
    if (t.isdst) {
      bool found = false;
      for (size_t __j = i; __j-- > 0 && !found;)
        if (!tt[__tr[__j].type].isdst)
          save = (t.utoff - tt[__tr[__j].type].utoff) / 60, found = true;
      for (size_t __j = i + 1; __j < __tr.size() && !found; ++__j)
        if (!tt[__tr[__j].type].isdst)
          save = (t.utoff - tt[__tr[__j].type].utoff) / 60, found = true;
      if (save == 0)
        save = 60;
    }
    const __period pd{__tr[i].at, t.utoff, save, t.abbrev};
    if (!periods.empty() && periods.back().offset == pd.offset && periods.back().save == pd.save &&
        periods.back().abbrev == pd.abbrev)
      continue;
    periods.push_back(pd);
  }

  // The footer.
  if (h.version >= '2' && p < e && *p == '\n') {
    const char* __fb = reinterpret_cast<const char*>(p + 1);
    const char* fe = static_cast<const char*>(::memchr(__fb, '\n', static_cast<size_t>(e - (p + 1))));
    if (fe != nullptr && fe != __fb && parse_posix_tz(string_view(__fb, static_cast<size_t>(fe - __fb)), rule)) {
      has_footer = true;
      std_abbrev = abbrev_index(rule.std_abbrev);
      dst_abbrev = rule.has_dst ? abbrev_index(rule.dst_abbrev) : std_abbrev;
    }
  }
  return true;
}

void __ycxx::__detail::__tz_data::load() {
  string __bytes;
  if (!read_file(zoneinfo_dir() + "/" + name, __bytes))
    return;
  ok = load_tzif(__bytes);
}

info __ycxx::__detail::__tz_data::rule_info(int64_t t) const {
  if (!rule.has_dst)
    return {min_time, max_time, rule.std_off, 0, abbrevs[std_abbrev]};
  const int64_t save = (rule.dst_off - rule.std_off) / 60;
  const int64_t y = __ycxx::__detail::__civil_from_days(__ycxx::__detail::__chrono_floor_div(t, 86400)).y;
  // DST all year round: each year's DST lasts until the next year's begins (Africa/Casablanca's
  // "<+00>0<+01>,0/0,J365/25"); one period without end, not one per year.
  bool permanent = true;
  for (int64_t __yy = y - 1; __yy <= y; ++__yy) {
    const int64_t __s0 = rule_local_time(rule.start, __yy) - rule.std_off;
    const int64_t e0 = rule_local_time(rule.end, __yy) - rule.dst_off;
    permanent = permanent && __s0 < e0 && e0 >= rule_local_time(rule.start, __yy + 1) - rule.std_off;
  }
  if (permanent)
    return {min_time, max_time, rule.dst_off, save, abbrevs[dst_abbrev]};
  struct edge {
    int64_t at;
    bool __dst;
  };
  edge edges[6];
  int n = 0;
  for (int64_t __yy = y - 1; __yy <= y + 1; ++__yy) {
    edges[n++] = {rule_local_time(rule.start, __yy) - rule.std_off, true};
    edges[n++] = {rule_local_time(rule.end, __yy) - rule.dst_off, false};
  }
  std::sort(edges, edges + n, [](const edge& a, const edge& b) { return a.at != b.at ? a.at < b.at : a.__dst < b.__dst; });
  // The state at t is that of the last edge at or before it; the period ends at the next edge
  // that changes the state.
  int k = -1;
  for (int i = 0; i < n; ++i)
    if (edges[i].at <= t)
      k = i;
  const bool __dst = k >= 0 ? edges[k].__dst : !edges[0].__dst;
  int64_t begin = min_time;
  for (int i = k; i >= 0; --i) {
    if (edges[i].__dst != __dst)
      break;
    begin = edges[i].at;
    if (i == 0 || edges[i - 1].__dst != __dst)
      break;
  }
  int64_t end = max_time;
  for (int i = k + 1; i < n; ++i)
    if (edges[i].__dst != __dst) {
      end = edges[i].at;
      break;
    }
  if (__dst)
    return {begin, end, rule.dst_off, save, abbrevs[dst_abbrev]};
  return {begin, end, rule.std_off, 0, abbrevs[std_abbrev]};
}

info __ycxx::__detail::__tz_data::sys_info(int64_t t) const {
  // The last period that begins at or before t.
  const auto __it = std::upper_bound(periods.begin(), periods.end(), t,
                                   [](int64_t __v, const __period& pd) { return __v < pd.begin; });
  const size_t k = static_cast<size_t>(__it - periods.begin()) - 1;
  const __period& pd = periods[k];
  if (has_footer && k + 1 == periods.size()) {
    // The last period continues into the footer's era; its values should agree with the
    // footer's at footer_from.
    if (t >= footer_from) {
      info r = rule_info(t);
      if (r.begin <= footer_from)
        r.begin = pd.begin;
      return r;
    }
    return {pd.begin, rule_info(footer_from).end, pd.offset, pd.save, abbrevs[pd.abbrev]};
  }
  const int64_t end = k + 1 < periods.size() ? periods[k + 1].begin : max_time;
  return {pd.begin, end, pd.offset, pd.save, abbrevs[pd.abbrev]};
}

namespace {

__chr::sys_seconds to_sys(int64_t t) { return __chr::sys_seconds(__chr::seconds(t)); }

void fill(__chr::sys_info& out, const info& i) {
  out.begin = to_sys(i.begin);
  out.end = to_sys(i.end);
  out.offset = __chr::seconds(i.offset);
  out.save = __chr::minutes(i.save);
  out.abbrev.assign(i.abbrev.data(), i.abbrev.size());
}

__ycxx::__detail::__tz_data* loaded(const __chr::time_zone& __tz) {
  __ycxx::__detail::__tz_data* d = __tz.data(__ycxx::__detail::__tz_ctor_tag{});
  std::call_once(d->once, [d] {
    try {
      d->load();
    } catch (...) {
      d->ok = false;
    }
  });
  return d->ok ? d : nullptr;
}

} // namespace

bool __ycxx::__detail::__tz_get_sys_info(const __chr::time_zone& __tz, __chr::sys_seconds __st, __chr::sys_info& out) {
  const __tz_data* d = loaded(__tz);
  if (d == nullptr)
    return false;
  fill(out, d->sys_info(__st.time_since_epoch().count()));
  return true;
}

bool __ycxx::__detail::__tz_get_local_info(const __chr::time_zone& __tz, __chr::local_seconds lt, __chr::local_info& out) {
  const __tz_data* d = loaded(__tz);
  if (d == nullptr)
    return false;
  // Offsets stay within a day and a few hours: look at the periods around lt - 30h .. lt + 30h.
  const int64_t __l = lt.time_since_epoch().count();
  const int64_t window = 30 * 3600;
  info found[2];
  int nfound = 0;
  info before{}, __after{};
  bool has_before = false, has_after = false;
  info i = d->sys_info(sat_add(__l, -window));
  for (int __guard = 0; __guard < 64; ++__guard) {
    const int64_t __lb = i.begin == min_time ? min_time : sat_add(i.begin, i.offset);
    const int64_t __le = i.end == max_time ? max_time : sat_add(i.end, i.offset);
    if (__lb <= __l && __l < __le) {
      if (nfound < 2)
        found[nfound++] = i;
    } else if (__le <= __l) {
      before = i, has_before = true;
    } else if (!has_after) {
      __after = i, has_after = true;
    }
    if (i.end == max_time || i.end > sat_add(__l, window))
      break;
    i = d->sys_info(i.end);
  }
  out = __chr::local_info{};
  if (nfound == 1) {
    out.result = __chr::local_info::unique;
    fill(out.first, found[0]);
  } else if (nfound == 2) {
    out.result = __chr::local_info::ambiguous;
    fill(out.first, found[0]);
    fill(out.second, found[1]);
  } else if (has_before && has_after) {
    out.result = __chr::local_info::nonexistent;
    fill(out.first, before);
    fill(out.second, __after);
  } else { // not reachable with consistent data
    out.result = __chr::local_info::unique;
    fill(out.first, d->sys_info(__l));
  }
  return true;
}

// ---- the database -----------------------------------------------------------------------------

namespace {

using __ycxx::__detail::__tz_ctor_tag;

constexpr unsigned short builtin_leaps[][3] = {
    {1972, 7, 1}, {1973, 1, 1}, {1974, 1, 1}, {1975, 1, 1}, {1976, 1, 1}, {1977, 1, 1}, {1978, 1, 1},
    {1979, 1, 1}, {1980, 1, 1}, {1981, 7, 1}, {1982, 7, 1}, {1983, 7, 1}, {1985, 7, 1}, {1988, 1, 1},
    {1990, 1, 1}, {1991, 1, 1}, {1992, 7, 1}, {1993, 7, 1}, {1994, 7, 1}, {1996, 1, 1}, {1997, 7, 1},
    {1999, 1, 1}, {2006, 1, 1}, {2009, 1, 1}, {2012, 7, 1}, {2015, 7, 1}, {2017, 1, 1}};

int month_from_name(string_view m) {
  static constexpr const char* __names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  for (int i = 0; i < 12; ++i)
    if (m.size() >= 3 && ::strncmp(m.data(), __names[i], 3) == 0)
      return i + 1;
  return 0;
}

int64_t to_int(string_view s, bool& ok) {
  int64_t __v = 0;
  bool __neg = false;
  size_t i = 0;
  if (!s.empty() && (s[0] == '-' || s[0] == '+'))
    __neg = s[0] == '-', i = 1;
  if (i == s.size())
    ok = false;
  for (; i < s.size(); ++i) {
    if (s[i] < '0' || s[i] > '9') {
      ok = false;
      return 0;
    }
    __v = __v * 10 + (s[i] - '0');
  }
  return __neg ? -__v : __v;
}

// "Leap YEAR MONTH DAY HH:MM:SS CORR R/S" lines; date() is the first second after the insertion.
bool load_leapseconds(const string& __dir, vector<__chr::leap_second>& out) {
  string __text;
  if (!read_file(__dir + "/leapseconds", __text))
    return false;
  bool any = false;
  for_each_line(__text, [&](string_view line) {
    const vector<string_view> __f = fields(line);
    if (__f.size() < 6 || __f[0] != "Leap")
      return;
    bool ok = true;
    const int64_t y = to_int(__f[1], ok);
    const int m = month_from_name(__f[2]);
    const int64_t d = to_int(__f[3], ok);
    cursor c{__f[4]};
    int64_t hms = 0;
    if (!ok || m == 0 || !parse_hms(c, hms) || (__f[5] != "+" && __f[5] != "-"))
      return;
    const bool __positive = __f[5] == "+";
    const int64_t at = __days_from_civil(y, static_cast<unsigned>(m), static_cast<unsigned>(d)) * 86400 + hms +
                       (__positive ? 0 : 1);
    out.emplace_back(__ycxx::__detail::__leap_second_tag{}, to_sys(at), __chr::seconds(__positive ? 1 : -1));
    any = true;
  });
  return any;
}

// "NTP-seconds TAI-UTC" lines; the first entry (1972-01-01, 10 s) is not a leap second.
bool load_leap_seconds_list(const string& __dir, vector<__chr::leap_second>& out) {
  string __text;
  if (!read_file(__dir + "/leap-seconds.list", __text))
    return false;
  int64_t prev = 0;
  bool first = true, any = false;
  for_each_line(__text, [&](string_view line) {
    if (line.empty() || line[0] == '#')
      return;
    const vector<string_view> __f = fields(line);
    if (__f.size() < 2)
      return;
    bool ok = true;
    const int64_t ntp = to_int(__f[0], ok);
    const int64_t diff = to_int(__f[1], ok);
    if (!ok)
      return;
    if (!first && diff != prev) {
      out.emplace_back(__ycxx::__detail::__leap_second_tag{}, to_sys(ntp - 2208988800LL), __chr::seconds(diff - prev));
      any = true;
    }
    first = false;
    prev = diff;
  });
  return any;
}

void load_leaps(const string& __dir, vector<__chr::leap_second>& out) {
  if (!load_leapseconds(__dir, out)) {
    out.clear();
    if (!load_leap_seconds_list(__dir, out)) {
      out.clear();
      for (const auto& __l : builtin_leaps)
        out.emplace_back(__ycxx::__detail::__leap_second_tag{}, to_sys(__days_from_civil(__l[0], __l[1], __l[2]) * 86400),
                         __chr::seconds(1));
    }
  }
  std::sort(out.begin(), out.end());
}

string read_version(const string& __dir) {
  string __text;
  if (read_file(__dir + "/+VERSION", __text)) {
    string __v = __trim(__text);
    if (!__v.empty())
      return __v;
  }
  __file in((__dir + "/tzdata.zi").c_str());
  if (in.__f != nullptr) {
    char line[256];
    string __v;
    if (::fgets(line, sizeof line, in.__f) != nullptr && ::strncmp(line, "# version", 9) == 0)
      __v = __trim(line + 9);
    if (!__v.empty())
      return __v;
  }
  return "unknown";
}

bool is_tzif(const string& path) {
  __file in(path.c_str());
  char magic[4];
  return in.__f != nullptr && ::fread(magic, 1, 4, in.__f) == 4 && ::memcmp(magic, "TZif", 4) == 0;
}

// Without tzdata.zi: every TZif file below dir is a zone, every symbolic link to one a link.
void walk(const string& __dir, const string& __rel, vector<string>& zones, vector<std::pair<string, string>>& links,
          int depth) {
  if (depth > 8)
    return;
  DIR* d = ::opendir((__rel.empty() ? __dir : __dir + "/" + __rel).c_str());
  if (d == nullptr)
    return;
  while (const dirent* e = ::readdir(d)) {
    const string name = e->d_name;
    if (name.empty() || name[0] == '.' || name[0] == '+')
      continue;
    if (__rel.empty() && (name == "posix" || name == "right" || name == "posixrules" || name == "localtime"))
      continue;
    const string r = __rel.empty() ? name : __rel + "/" + name;
    const string __full = __dir + "/" + r;
    struct stat __ls;
    if (::lstat(__full.c_str(), &__ls) != 0)
      continue;
    if (S_ISDIR(__ls.st_mode)) {
      walk(__dir, r, zones, links, depth + 1);
    } else if (S_ISLNK(__ls.st_mode)) {
      char __buf[4096];
      const ssize_t n = ::readlink(__full.c_str(), __buf, sizeof __buf - 1);
      if (n <= 0 || !is_tzif(__full))
        continue;
      string target(__buf, static_cast<size_t>(n));
      if (target[0] == '/') {
        if (target.compare(0, __dir.size() + 1, __dir + "/") != 0)
          continue;
        target.erase(0, __dir.size() + 1);
      } else {
        string base = __rel;
        while (target.compare(0, 3, "../") == 0) {
          target.erase(0, 3);
          const size_t s = base.rfind('/');
          base = s == string::npos ? string() : base.substr(0, s);
        }
        if (!base.empty())
          target = base + "/" + target;
      }
      links.emplace_back(r, target);
    } else if (S_ISREG(__ls.st_mode) && is_tzif(__full)) {
      zones.push_back(r);
    }
  }
  ::closedir(d);
}

const __chr::time_zone* find_zone(const __chr::tzdb& __db, string_view name) noexcept {
  auto __z = std::lower_bound(__db.zones.begin(), __db.zones.end(), name,
                            [](const __chr::time_zone& a, string_view b) { return a.name() < b; });
  if (__z != __db.zones.end() && __z->name() == name)
    return &*__z;
  return nullptr;
}

// The database, or nullptr when there is none or memory runs out.
__chr::tzdb* load_tzdb() noexcept {
  __chr::tzdb* __db = nullptr;
  try {
    const string __dir = zoneinfo_dir();
    __db = new __chr::tzdb;
    __db->version = read_version(__dir);
    vector<string> zones;
    vector<std::pair<string, string>> links;
    string __text;
    if (read_file(__dir + "/tzdata.zi", __text)) {
      for_each_line(__text, [&](string_view line) {
        if (line.size() < 3 || line[1] != ' ')
          return;
        const vector<string_view> __f = fields(line);
        if (line[0] == 'Z' && __f.size() >= 2)
          zones.emplace_back(__f[1]);
        else if (line[0] == 'L' && __f.size() >= 3)
          links.emplace_back(string(__f[2]), string(__f[1]));
      });
    } else {
      walk(__dir, string(), zones, links, 0);
    }
    if (zones.empty()) {
      delete __db;
      return nullptr;
    }
    std::sort(zones.begin(), zones.end());
    zones.erase(std::unique(zones.begin(), zones.end()), zones.end());
    __db->zones.reserve(zones.size());
    for (string& __z : zones) {
      auto* data = new __ycxx::__detail::__tz_data(__z);
      __db->zones.emplace_back(__tz_ctor_tag{}, static_cast<string&&>(__z), data);
    }
    std::sort(links.begin(), links.end());
    links.erase(std::unique(links.begin(), links.end(),
                            [](const auto& a, const auto& b) { return a.first == b.first; }),
                links.end());
    for (auto& __l : links) {
      // A name that is also a zone stays a zone.
      if (find_zone(*__db, __l.first) != nullptr)
        continue;
      __db->links.emplace_back(__tz_ctor_tag{}, static_cast<string&&>(__l.first), static_cast<string&&>(__l.second));
    }
    load_leaps(__dir, __db->leap_seconds);
  } catch (...) {
    delete __db;
    return nullptr;
  }
  return __db;
}

std::mutex& list_mutex() {
  static std::mutex m;
  return m;
}

} // namespace

const __chr::time_zone* __ycxx::__detail::__tzdb_find(const __chr::tzdb& __db, string_view name) noexcept {
  if (const __chr::time_zone* __z = find_zone(__db, name))
    return __z;
  auto __l = std::lower_bound(__db.links.begin(), __db.links.end(), name,
                            [](const __chr::time_zone_link& a, string_view b) { return a.name() < b; });
  if (__l != __db.links.end() && __l->name() == name) {
    // A link may name another link (not in tzdata.zi, but possible in a directory tree).
    string_view target = __l->target();
    for (int hops = 0; hops < 8; ++hops) {
      if (const __chr::time_zone* __z = find_zone(__db, target))
        return __z;
      auto __l2 = std::lower_bound(__db.links.begin(), __db.links.end(), target,
                                 [](const __chr::time_zone_link& a, string_view b) { return a.name() < b; });
      if (__l2 == __db.links.end() || __l2->name() != target)
        break;
      target = __l2->target();
    }
  }
  return nullptr;
}

// The local zone: $TZ when it names a zone (optionally ":name" or a path into the zoneinfo
// directory), else the target of /etc/localtime below a "zoneinfo/" directory, else
// /etc/timezone, else UTC.
const __chr::time_zone* __ycxx::__detail::__tzdb_current(const __chr::tzdb& __db) noexcept {
  try {
    const string __dir = zoneinfo_dir();
    auto by_name = [&](string_view n) -> const __chr::time_zone* {
      if (!n.empty() && n[0] == ':')
        n.remove_prefix(1);
      if (n.size() > __dir.size() + 1 && n.compare(0, __dir.size(), __dir) == 0 && n[__dir.size()] == '/')
        n.remove_prefix(__dir.size() + 1);
      return n.empty() ? nullptr : __tzdb_find(__db, n);
    };
    if (const char* __tz = ::getenv("TZ"); __tz != nullptr && *__tz != '\0')
      if (const __chr::time_zone* __z = by_name(__tz))
        return __z;
    char __buf[4096];
    const ssize_t n = ::readlink("/etc/localtime", __buf, sizeof __buf - 1);
    if (n > 0) {
      const string_view target(__buf, static_cast<size_t>(n));
      if (const __chr::time_zone* __z = by_name(target))
        return __z;
      const size_t at = target.rfind("zoneinfo/");
      if (at != string_view::npos)
        if (const __chr::time_zone* __z = __tzdb_find(__db, target.substr(at + 9)))
          return __z;
    }
    string __text;
    if (read_file("/etc/timezone", __text))
      if (const __chr::time_zone* __z = __tzdb_find(__db, __trim(__text)))
        return __z;
    if (const __chr::time_zone* __z = __tzdb_find(__db, "UTC"))
      return __z;
    return __tzdb_find(__db, "Etc/UTC");
  } catch (...) {
    return nullptr;
  }
}

__chr::tzdb_list* __ycxx::__detail::__tzdb_list_instance() noexcept {
  // Built on first successful use and never destroyed: zones may be queried from static
  // destructors. A failed load (no database, or no memory) is not kept: the next call tries
  // again, and get_tzdb_list throws runtime_error meanwhile ([time.zone.db.access]/1).
  static constinit __chr::tzdb_list* list = nullptr; // written under list_mutex()
  if (__chr::tzdb_list* __l = __atomic_load_n(&list, __ATOMIC_ACQUIRE))
    return __l;
  try {
    std::lock_guard<std::mutex> lock(list_mutex());
    if (list != nullptr)
      return list;
    __chr::tzdb* __db = load_tzdb();
    if (__db == nullptr)
      return nullptr;
    auto* __l = new (std::nothrow) __chr::tzdb_list(__tz_ctor_tag{});
    auto* node = __l == nullptr ? nullptr : new (std::nothrow) __tzdb_node{static_cast<__chr::tzdb&&>(*__db), nullptr};
    delete __db;
    if (node == nullptr) {
      delete __l;
      return nullptr;
    }
    __l->push_front(__tz_ctor_tag{}, node);
    __atomic_store_n(&list, __l, __ATOMIC_RELEASE);
    return __l;
  } catch (...) { // the mutex (system_error)
    return nullptr;
  }
}

const __chr::tzdb* __ycxx::__detail::__tzdb_reload() noexcept {
  __chr::tzdb_list* __l = __tzdb_list_instance();
  if (__l == nullptr)
    return nullptr;
  try {
    std::lock_guard<std::mutex> lock(list_mutex());
    const string __dir = zoneinfo_dir();
    if (read_version(__dir) == __l->front().version)
      return &__l->front();
    __chr::tzdb* __db = load_tzdb();
    if (__db == nullptr)
      return nullptr;
    auto* node = new __tzdb_node{static_cast<__chr::tzdb&&>(*__db), nullptr};
    delete __db;
    __l->push_front(__tz_ctor_tag{}, node);
    return &node->__db;
  } catch (...) {
    return nullptr;
  }
}

std::string __ycxx::__detail::__tzdb_remote_version() { return read_version(zoneinfo_dir()); }

const __chr::tzdb* __ycxx::__detail::__tzdb_erase_after(__chr::tzdb_list& list, const __tzdb_node* p) noexcept {
  std::lock_guard<std::mutex> lock(list_mutex());
  (void)list;
  __tzdb_node* victim = p->next;
  if (victim == nullptr)
    return nullptr;
  __atomic_store_n(&const_cast<__tzdb_node*>(p)->next, victim->next, __ATOMIC_RELEASE);
  delete victim;
  return p->next != nullptr ? &p->next->__db : nullptr;
}
