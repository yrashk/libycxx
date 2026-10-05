// libycxx hosted: the text forms of <chrono> values: the formatters ([time.format]), the stream
// inserters of every chrono type, local_time_format, and the constructors of the time zone
// exception classes ([time.zone.exception]). Parsing is in ycxx/hosted/chrono_parse.hpp.
//
// Every formatter turns its value into one set of fields (date, time of day, zone) and runs the
// chrono-specs over them into a local buffer, which is then padded as a whole. Without the L
// option the "C" locale's names and decimal point are built in; with it, the locale-dependent
// conversions (%a %A %b %B %c %p %r %x %X and the E/O-modified ones) go through the formatting
// locale's time_put facet (a call into the hosted runtime, src/hosted/chrono.cpp) unless that is
// the classic locale's facet, and %S and the counts of durations take the locale's numpunct.
// Without chrono-specs a value is formatted as its stream inserter would write it
// ([time.format]/7). The stream inserters write the same text with the stream's
// locale, so they need no <sstream>.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/charconv.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/core/chrono_cal.hpp>
#include <ycxx/core/format_base.hpp>
#include <ycxx/core/format_unicode.hpp>
#include <ycxx/core/iosfwd.hpp>
#include <ycxx/hosted/chrono_tz.hpp>
#include <ycxx/hosted/format_locale.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// [time.format]: local-time-format-t.
template <class Duration>
struct local_time_format_t {
  std::chrono::local_time<Duration> time_;
  const std::string* abbrev_;
  const std::chrono::seconds* offset_sec_;
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace chrono {
template <class Duration>
ycxx::adl_free::local_time_format_t<Duration> local_time_format(local_time<Duration> time,
                                                                const string* abbrev = nullptr,
                                                                const seconds* offset_sec = nullptr) {
  return {time, abbrev, offset_sec};
}
}} // namespace std::chrono

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// ---- the fields of a value ------------------------------------------------------------------

// The information a type provides ([time.format]/3, /6).
enum chrono_info : unsigned {
  ci_year = 1,
  ci_month = 2,
  ci_day = 4,
  ci_weekday = 8,
  ci_date = 16, // a complete date: day of the year, week numbers
  ci_time = 32, // a time of day (or a duration read as one)
  ci_duration = 64,
  ci_zone = 128,   // %Z
  ci_offset = 256, // %z
  ci_full_date = ci_year | ci_month | ci_day | ci_weekday | ci_date,
};

template <class charT>
struct chrono_fields {
  int year = 0;
  unsigned month = 0, day = 0, weekday = 0;
  int yday = 0; // 0 for January 1st
  bool weekday_ok = false;    // weekday is a valid one (0-6)
  bool weekday_known = false; // weekday holds an encoding, valid or not
  bool date_ok = false; // year, month, day, weekday and yday form a valid date
  bool negative = false;
  unsigned long long hours = 0;
  unsigned minutes = 0, seconds = 0;
  unsigned long long subseconds = 0;
  unsigned width = 0; // fractional digits of the seconds
  bool is_duration = false;
  bool has_abbrev = false, has_offset = false;
  std::string_view abbrev;
  long long offset = 0; // seconds
  // %Q and %q of a duration
  char count[64] = {};
  std::size_t count_len = 0;
  charT suffix[48] = {};
  std::size_t suffix_len = 0;
};

template <class charT>
constexpr void chrono_set_days(chrono_fields<charT>& f, long long z) noexcept {
  const civil_date c = ::ycxx::detail::civil_from_days(z);
  f.year = c.y;
  f.month = c.m;
  f.day = c.d;
  f.weekday = ::ycxx::detail::weekday_from_days(z);
  f.weekday_ok = f.weekday_known = f.date_ok = true;
  f.yday = static_cast<int>(::ycxx::detail::wrap_sub(z, ::ycxx::detail::days_from_civil(c.y, 1, 1)));
}

// y/m/d as given (not necessarily a valid date); the weekday only for a valid one.
template <class charT>
constexpr void chrono_set_ymd(chrono_fields<charT>& f, const std::chrono::year_month_day& ymd) noexcept {
  if (ymd.ok()) {
    ::ycxx::detail::chrono_set_days(f, std::chrono::sys_days(ymd).time_since_epoch().count());
    return;
  }
  f.year = static_cast<int>(ymd.year());
  f.month = static_cast<unsigned>(ymd.month());
  f.day = static_cast<unsigned>(ymd.day());
  if (f.month >= 1 && f.month <= 12)
    f.yday = ::ycxx::detail::days_from_civil(f.year, f.month, f.day) - ::ycxx::detail::days_from_civil(f.year, 1, 1);
}

// ---- splitting a count into units ------------------------------------------------------------
// Without hh_mm_ss and duration_cast, whose ratio arithmetic overflows for fine periods (atto
// to hours) and whose negation overflows for the most negative count: the magnitude of the
// count is split by the period's num and den with 128-bit intermediate products.

// floor(a * b / c) (its low 64 bits) and a * b mod c, for c > 0.
struct chrono_qr {
  unsigned long long q, r;
};
template <class U = uint128>
constexpr chrono_qr chrono_muldiv(unsigned long long a, unsigned long long b, unsigned long long c) noexcept {
  if constexpr (cfg::has_int128) {
    const U p = static_cast<U>(a) * b;
    return {static_cast<unsigned long long>(p / c), static_cast<unsigned long long>(p % c)};
  } else {
    // The 128-bit product, then restoring division one bit at a time.
    const unsigned long long a0 = a & 0xffffffffu, a1 = a >> 32, b0 = b & 0xffffffffu, b1 = b >> 32;
    const unsigned long long p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
    const unsigned long long mid = (p00 >> 32) + (p01 & 0xffffffffu) + (p10 & 0xffffffffu);
    const unsigned long long hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32), lo = (mid << 32) | (p00 & 0xffffffffu);
    unsigned long long q = 0, r = 0;
    for (int i = 127; i >= 0; --i) {
      const bool carry = (r >> 63) != 0;
      r = (r << 1) | (((i >= 64 ? hi >> (i - 64) : lo >> i)) & 1u);
      q <<= 1;
      if (carry || r >= c) {
        r -= c;
        q |= 1;
      }
    }
    return {q, r};
  }
}

// m ticks of num/den s as `whole` units of `unit` s (low 64 bits), `secs` (< unit) whole seconds
// and `rem` (< den) ticks of 1/den s.
template <class T>
struct chrono_parts {
  unsigned long long whole, secs;
  T rem;
};
constexpr chrono_parts<unsigned long long> chrono_split(unsigned long long m, unsigned long long num,
                                                        unsigned long long den, unsigned long long unit) noexcept {
  // m * num / den = (m / den) * num + (m % den) * num / den
  const chrono_qr a = ::ycxx::detail::chrono_muldiv(m / den, num, unit);
  const chrono_qr b = ::ycxx::detail::chrono_muldiv(m % den, num, den); // b.q < num
  const unsigned long long t = a.r + b.q;
  return {a.q + t / unit, t % unit, b.r};
}

// Floating-point counts: the same steps in long double (exact for integral values).
constexpr long double chrono_floor(long double x) noexcept { // x >= 0
  return x < 9.2e18L ? static_cast<long double>(static_cast<unsigned long long>(x)) : x;
}
constexpr unsigned long long chrono_to_ull(long double x) noexcept { // x >= 0
  return x < 18446744073709551615.0L ? static_cast<unsigned long long>(x) : ~0ull;
}
constexpr chrono_parts<long double> chrono_split(long double m, unsigned long long num, unsigned long long den,
                                                 unsigned long long unit) noexcept {
  const long double n = static_cast<long double>(num), d = static_cast<long double>(den),
                    u = static_cast<long double>(unit);
  auto mod = [](long double x, long double y, long double q) { // x - q * y, in [0, y)
    const long double r = x - q * y;
    return r < 0 ? 0 : r >= y ? y - 1 : r;
  };
  const long double q = ::ycxx::detail::chrono_floor(m / d), r = mod(m, d, q);
  const long double a = q * n, aq = ::ycxx::detail::chrono_floor(a / u), ar = mod(a, u, aq);
  const long double rn = r * n, bq = ::ycxx::detail::chrono_floor(rn / d), br = mod(rn, d, bq);
  const long double t = ar + bq, tq = ::ycxx::detail::chrono_floor(t / u);
  return {::ycxx::detail::chrono_to_ull(aq + tq), ::ycxx::detail::chrono_to_ull(mod(t, u, tq)), br};
}

// rem ticks of 1/den s as `width` fractional digits (truncated).
constexpr unsigned long long chrono_frac(unsigned long long rem, unsigned long long den, unsigned width) noexcept {
  unsigned long long p = 1;
  for (unsigned i = 0; i != width; ++i)
    p *= 10;
  return ::ycxx::detail::chrono_muldiv(rem, p, den).q;
}
constexpr unsigned long long chrono_frac(long double rem, unsigned long long den, unsigned width) noexcept {
  long double p = 1;
  for (unsigned i = 0; i != width; ++i)
    p *= 10;
  return ::ycxx::detail::chrono_to_ull(::ycxx::detail::chrono_floor(rem * p / static_cast<long double>(den)));
}

// The counts split without hh_mm_ss: the standard integer types up to 64 bits and the floating-
// point types. Others (wider integers, class types emulating arithmetic) go through hh_mm_ss.
template <class Rep>
inline constexpr bool chrono_plain_rep =
    (std::is_integral_v<Rep> && sizeof(Rep) <= sizeof(long long)) || std::is_floating_point_v<Rep>;

// |c| as unsigned long long or long double (a NaN as 0).
template <class Rep>
constexpr auto chrono_magnitude(Rep c) noexcept {
  if constexpr (std::is_floating_point_v<Rep>) {
    const long double x = static_cast<long double>(c);
    return x < 0 ? -x : x > 0 ? x : 0.0L;
  } else if constexpr (std::is_signed_v<Rep>) {
    const long long v = static_cast<long long>(c);
    return v < 0 ? 0ull - static_cast<unsigned long long>(v) : static_cast<unsigned long long>(v);
  } else {
    return static_cast<unsigned long long>(c);
  }
}

// The time of day (or duration) d: hours (not reduced modulo 24), minutes, seconds and the
// fractional digits of hh_mm_ss<Dur>::fractional_width ([time.hms.members]).
template <class charT, class Rep, class Period>
constexpr void chrono_set_time(chrono_fields<charT>& f, const std::chrono::duration<Rep, Period>& d) {
  using Dur = std::chrono::duration<Rep, Period>;
  if constexpr (chrono_plain_rep<Rep>) {
    using P = typename Period::type;
    constexpr unsigned width = ::ycxx::detail::hms_fractional_width(P::den);
    f.negative = d.count() < Rep(0);
    const auto parts = ::ycxx::detail::chrono_split(::ycxx::detail::chrono_magnitude(d.count()),
                                                    static_cast<unsigned long long>(P::num),
                                                    static_cast<unsigned long long>(P::den), 3600);
    f.hours = parts.whole;
    f.minutes = static_cast<unsigned>(parts.secs / 60);
    f.seconds = static_cast<unsigned>(parts.secs % 60);
    f.width = width;
    f.subseconds = ::ycxx::detail::chrono_frac(parts.rem, static_cast<unsigned long long>(P::den), width);
  } else {
    const std::chrono::hh_mm_ss<Dur> h(d);
    using prec = typename std::chrono::hh_mm_ss<Dur>::precision;
    f.negative = h.is_negative();
    f.hours = static_cast<unsigned long long>(h.hours().count());
    f.minutes = static_cast<unsigned>(h.minutes().count());
    f.seconds = static_cast<unsigned>(h.seconds().count());
    f.width = std::chrono::hh_mm_ss<Dur>::fractional_width;
    if constexpr (std::chrono::treat_as_floating_point_v<typename prec::rep>)
      f.subseconds = static_cast<unsigned long long>(
          std::chrono::duration_cast<std::chrono::duration<long long, typename prec::period>>(h.subseconds()).count());
    else
      f.subseconds = static_cast<unsigned long long>(h.subseconds().count());
  }
}

// A time point's date and time of day, from its time since the epoch plus `days` days.
template <class charT, class Rep, class Period>
constexpr void chrono_set_point(chrono_fields<charT>& f, const std::chrono::duration<Rep, Period>& since_epoch,
                                long long days = 0) {
  using Duration = std::chrono::duration<Rep, Period>;
  if constexpr (chrono_plain_rep<Rep>) {
    // The magnitude in whole days, seconds and ticks; a negative one is floored.
    using P = typename Period::type;
    constexpr unsigned long long den = static_cast<unsigned long long>(P::den);
    constexpr unsigned width = ::ycxx::detail::hms_fractional_width(P::den);
    auto parts = ::ycxx::detail::chrono_split(::ycxx::detail::chrono_magnitude(since_epoch.count()),
                                              static_cast<unsigned long long>(P::num), den, 86400);
    long long day = static_cast<long long>(parts.whole);
    if (since_epoch.count() < Rep(0)) {
      day = -day;
      if (parts.secs != 0 || parts.rem != 0) {
        --day;
        if (parts.rem != 0) {
          parts.rem = static_cast<decltype(parts.rem)>(den) - parts.rem;
          parts.secs = 86399 - parts.secs;
        } else {
          parts.secs = 86400 - parts.secs;
        }
      }
    }
    ::ycxx::detail::chrono_set_days(f, static_cast<long long>(::ycxx::detail::wrap_add(day, days)));
    f.hours = parts.secs / 3600;
    f.minutes = static_cast<unsigned>(parts.secs / 60 % 60);
    f.seconds = static_cast<unsigned>(parts.secs % 60);
    f.width = width;
    f.subseconds = ::ycxx::detail::chrono_frac(parts.rem, den, width);
  } else {
    // Day and time of day by a floored division of the count, which cannot overflow even for
    // time_point::min() (floor<days> would compare in the finer type).
    using cd = std::common_type_t<Duration, std::chrono::days>;
    const cd c(since_epoch);
    const auto ticks = cd(std::chrono::days(1)).count();
    auto q = c.count() / ticks;
    auto r = c.count() % ticks;
    if (r < 0) {
      --q;
      r += ticks;
    }
    const long long day = static_cast<long long>(::ycxx::detail::wrap_add(static_cast<long long>(q), days));
    ::ycxx::detail::chrono_set_days(f, day);
    ::ycxx::detail::chrono_set_time(f, cd(r));
  }
}

// [time.duration.io]/1: the units suffix.
template <class charT>
struct chrono_suffix_text {
  charT text[48];
  std::size_t len;
};
template <class Period, class charT>
constexpr chrono_suffix_text<charT> chrono_suffix() noexcept {
  chrono_suffix_text<charT> r{};
  auto put = [&r](const char* s) {
    for (; *s != 0; ++s)
      r.text[r.len++] = static_cast<charT>(*s);
  };
  using P = typename Period::type;
  using std::ratio;
  if constexpr (__is_same(P, std::atto))
    put("as");
  else if constexpr (__is_same(P, std::femto))
    put("fs");
  else if constexpr (__is_same(P, std::pico))
    put("ps");
  else if constexpr (__is_same(P, std::nano))
    put("ns");
  else if constexpr (__is_same(P, std::micro)) {
    // "µs" where literals are Unicode, else "us".
    if constexpr (uni::encoding<charT> == 8) {
      r.text[r.len++] = static_cast<charT>(0xC2);
      r.text[r.len++] = static_cast<charT>(0xB5);
      put("s");
    } else if constexpr (uni::encoding<charT> != 0) {
      r.text[r.len++] = static_cast<charT>(0xB5);
      put("s");
    } else {
      put("us");
    }
  } else if constexpr (__is_same(P, std::milli))
    put("ms");
  else if constexpr (__is_same(P, std::centi))
    put("cs");
  else if constexpr (__is_same(P, std::deci))
    put("ds");
  else if constexpr (__is_same(P, ratio<1>))
    put("s");
  else if constexpr (__is_same(P, std::deca))
    put("das");
  else if constexpr (__is_same(P, std::hecto))
    put("hs");
  else if constexpr (__is_same(P, std::kilo))
    put("ks");
  else if constexpr (__is_same(P, std::mega))
    put("Ms");
  else if constexpr (__is_same(P, std::giga))
    put("Gs");
  else if constexpr (__is_same(P, std::tera))
    put("Ts");
  else if constexpr (__is_same(P, std::peta))
    put("Ps");
  else if constexpr (__is_same(P, std::exa))
    put("Es");
  else if constexpr (__is_same(P, ratio<60>))
    put("min");
  else if constexpr (__is_same(P, ratio<3600>))
    put("h");
  else if constexpr (__is_same(P, ratio<86400>))
    put("d");
  else {
    char digits[48];
    char* const end = digits + sizeof digits;
    char* p = end;
    *--p = 0;
    *--p = 's';
    *--p = ']';
    if constexpr (P::den != 1) {
      p = ::ycxx::detail::charconv_write_unsigned(p, static_cast<unsigned long long>(P::den), 10);
      *--p = '/';
    }
    p = ::ycxx::detail::charconv_write_unsigned(p, static_cast<unsigned long long>(P::num), 10);
    *--p = '[';
    put(p);
  }
  return r;
}

// %Q: the count of a duration's magnitude, as "{}" formats it.
template <class charT, class Rep, class Period>
constexpr void chrono_set_duration(chrono_fields<charT>& f, const std::chrono::duration<Rep, Period>& d) {
  f.is_duration = true;
  ::ycxx::detail::chrono_set_time(f, d);
  const chrono_suffix_text<charT> s = ::ycxx::detail::chrono_suffix<Period, charT>();
  for (std::size_t i = 0; i != s.len; ++i)
    f.suffix[i] = s.text[i];
  f.suffix_len = s.len;
  const Rep c = d.count();
  if constexpr (std::chrono::treat_as_floating_point_v<Rep>) {
    const auto r = std::to_chars(f.count, f.count + sizeof f.count, c < Rep(0) ? -c : c);
    f.count_len = static_cast<std::size_t>(r.ptr - f.count);
  } else if constexpr (std::is_signed_v<Rep>) {
    const unsigned long long m = c < Rep(0) ? 0ull - static_cast<unsigned long long>(static_cast<long long>(c))
                                            : static_cast<unsigned long long>(c);
    char* p = ::ycxx::detail::charconv_write_unsigned(f.count + sizeof f.count, m, 10);
    f.count_len = static_cast<std::size_t>(f.count + sizeof f.count - p);
    for (std::size_t i = 0; i != f.count_len; ++i)
      f.count[i] = p[i];
  } else {
    char* p = ::ycxx::detail::charconv_write_unsigned(f.count + sizeof f.count, static_cast<unsigned long long>(c), 10);
    f.count_len = static_cast<std::size_t>(f.count + sizeof f.count - p);
    for (std::size_t i = 0; i != f.count_len; ++i)
      f.count[i] = p[i];
  }
}

// ---- the locale-dependent conversions (src/hosted/chrono.cpp) ---------------------------------

struct chrono_c_tm {
  int sec, min, hour, mday, mon, year, wday, yday;
};
// Appends what loc's time_put<charT> writes for %<mod><spec> of t.
void chrono_put_localized(std::string& out, const std::locale& loc, const chrono_c_tm& t, char spec, char mod);
void chrono_put_localized(std::wstring& out, const std::locale& loc, const chrono_c_tm& t, char spec, char mod);
// Whether loc's time_put<charT> is the classic locale's facet (every supported named locale,
// "C", "POSIX", "C.UTF-8", shares it), whose conventions are the "C" locale's.
bool chrono_classic_time_put(const std::locale& loc, char);
bool chrono_classic_time_put(const std::locale& loc, wchar_t);

inline constexpr const char* chrono_weekday_names[7] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                                                        "Thursday", "Friday", "Saturday"};
inline constexpr const char* chrono_month_names[12] = {"January", "February", "March",     "April",
                                                       "May",     "June",     "July",      "August",
                                                       "September", "October", "November", "December"};

// ---- writing ----------------------------------------------------------------------------------

// [time.format]/2: the formatting locale is the "C" locale without the L option. With it, the
// locale's numpunct gives %S its decimal point and the counts of the default duration format
// their digit grouping, and its time_put writes the locale-dependent conversions (names, %c %x
// %X %r %p and the E/O forms), unless it is the classic facet: the "C" locale's conventions are
// then built in (as without L). That keeps "{:L...}" with the "C" locale identical to "{:...}"
// also where a C tm cannot carry the value (hours of a duration beyond 23, years before 1 or
// after 9999, whose %Y the chrono formatter pads to four digits while strftime does not).
template <class charT>
struct chrono_out {
  fmt_dynbuf<charT>& b;
  const std::locale* loc; // the formatting locale when the L option is given, else null ("C")
  bool own_time = false;  // loc has a time_put of its own (not the classic one)

  chrono_out(fmt_dynbuf<charT>& buf, const std::locale* l)
      : b(buf), loc(l), own_time(l != nullptr && !::ycxx::detail::chrono_classic_time_put(*l, charT())) {}

  void ch(char c) { b.push_back(static_cast<charT>(c)); }
  void ascii(const char* s, std::size_t n) {
    for (std::size_t i = 0; i != n; ++i)
      b.push_back(static_cast<charT>(s[i]));
  }
  void ascii(const char* s) {
    for (; *s != 0; ++s)
      b.push_back(static_cast<charT>(*s));
  }
  // v in decimal with at least `digits` digits, padded with `pad`.
  void num(unsigned long long v, int digits, char pad = '0') {
    char d[24];
    char* const end = d + sizeof d;
    char* p = ::ycxx::detail::charconv_write_unsigned(end, v, 10);
    for (int n = static_cast<int>(end - p); n < digits; ++n)
      ch(pad);
    ascii(p, static_cast<std::size_t>(end - p));
  }
  void snum(long long v, int digits) {
    if (v < 0)
      ch('-');
    num(v < 0 ? 0ull - static_cast<unsigned long long>(v) : static_cast<unsigned long long>(v), digits);
  }
  charT decimal_point() const {
    if (loc == nullptr)
      return charT('.');
    return std::use_facet<std::numpunct<charT>>(*loc).decimal_point();
  }
  // [p, e): a count as num_put writes it with loc ([facet.num.put.virtuals] stage 2: digit
  // groups of the integer part separated by thousands_sep, the locale's decimal point), or as
  // is without L.
  void count(const char* p, const char* e) {
    if (loc == nullptr)
      return ascii(p, static_cast<std::size_t>(e - p));
    const std::numpunct<charT>& np = std::use_facet<std::numpunct<charT>>(*loc);
    const std::string g = np.grouping();
    const charT sep = np.thousands_sep();
    if (p != e && *p == '-')
      ch(*p++);
    const char* d = p;
    while (d != e && *d >= '0' && *d <= '9')
      ++d;
    // sep_before[i]: a separator precedes integer digit i
    bool sep_before[128] = {};
    const std::size_t n = static_cast<std::size_t>(d - p);
    for (std::size_t t = 0, done = 0; n <= 128;) {
      const std::size_t sz = ::ycxx::detail::fmt_group_size(g, t++);
      if (sz == 0 || done + sz >= n)
        break;
      done += sz;
      sep_before[n - done] = true;
    }
    for (std::size_t i = 0; i != n; ++i) {
      if (sep_before[i])
        b.push_back(sep);
      ch(p[i]);
    }
    for (; d != e; ++d)
      *d == '.' ? b.push_back(np.decimal_point()) : ch(*d);
  }
  void localized(const chrono_fields<charT>& f, char spec, char mod) {
    // The hour count of a duration where a tm can hold it (as %H writes it without L), reduced
    // modulo 24 for the 12-hour forms and AM/PM.
    const bool twelve = spec == 'I' || spec == 'p' || spec == 'r';
    const unsigned long long h =
        twelve || f.hours > static_cast<unsigned long long>(__INT_MAX__) ? f.hours % 24 : f.hours;
    const chrono_c_tm t{static_cast<int>(f.seconds),
                        static_cast<int>(f.minutes),
                        static_cast<int>(h),
                        static_cast<int>(f.day),
                        static_cast<int>(f.month) - 1,
                        f.year - 1900,
                        static_cast<int>(f.weekday),
                        f.yday};
    std::basic_string<charT> s;
    ::ycxx::detail::chrono_put_localized(s, *loc, t, spec, mod);
    b.append(s.data(), s.size());
  }
};

[[noreturn]] inline void chrono_missing(const char* what) { ::ycxx::detail::throw_format_error(what); }

constexpr unsigned chrono_iso_weeks(int y) noexcept {
  const unsigned wd = ::ycxx::detail::weekday_from_days(::ycxx::detail::days_from_civil(y, 1, 1));
  return wd == 4 || (wd == 3 && ::ycxx::detail::is_leap_year(y)) ? 53u : 52u;
}
// The ISO 8601 week-based year and week of f's date.
template <class charT>
constexpr void chrono_iso_week(const chrono_fields<charT>& f, int& year, unsigned& week) noexcept {
  const int wd_mon = static_cast<int>((f.weekday + 6) % 7);
  int w = (f.yday - wd_mon + 10) / 7;
  year = f.year;
  if (w < 1) {
    --year;
    w = static_cast<int>(::ycxx::detail::chrono_iso_weeks(year));
  } else if (w > static_cast<int>(::ycxx::detail::chrono_iso_weeks(year))) {
    ++year;
    w = 1;
  }
  week = static_cast<unsigned>(w);
}

template <class charT, class PC>
void chrono_write_specs(chrono_out<charT>& o, const PC* p, std::type_identity_t<const PC*> e,
                        const chrono_fields<charT>& f, bool sign);

// f without its subseconds: the locale's representations %c, %r and %X show whole seconds.
template <class charT>
constexpr chrono_fields<charT> chrono_whole_seconds(chrono_fields<charT> f) noexcept {
  f.width = 0;
  return f;
}

// One conversion specifier.
template <class charT>
void chrono_write_one(chrono_out<charT>& o, const chrono_fields<charT>& f, char spec, char mod) {
  const bool L = o.own_time;
  auto weekday_name = [&](bool full) {
    if (!f.weekday_ok || f.weekday > 6)
      ::ycxx::detail::chrono_missing("std::format: the value does not contain a valid weekday");
    if (L)
      return o.localized(f, full ? 'A' : 'a', 0);
    const char* n = chrono_weekday_names[f.weekday];
    full ? o.ascii(n) : o.ascii(n, 3);
  };
  auto month_name = [&](bool full) {
    if (f.month < 1 || f.month > 12)
      ::ycxx::detail::chrono_missing("std::format: the value does not contain a valid month");
    if (L)
      return o.localized(f, spec, 0); // %b, %h or %B, as written
    const char* n = chrono_month_names[f.month - 1];
    full ? o.ascii(n) : o.ascii(n, 3);
  };
  // The day of the year and the week numbers need a date ([time.format]/3: "If the formatted
  // object does not contain the information the conversion specifier refers to, an exception
  // of type format_error is thrown"); a !ok() year_month_day and the like name no day.
  auto valid_date = [&] {
    if (!f.date_ok)
      ::ycxx::detail::chrono_missing("std::format: the value does not contain a valid date");
  };
  auto secs = [&](bool fraction) {
    if (L && mod == 'O')
      o.localized(f, 'S', 'O');
    else
      o.num(f.seconds, 2);
    if (fraction && f.width != 0) {
      o.b.push_back(o.decimal_point());
      o.num(f.subseconds, static_cast<int>(f.width));
    }
  };
  // The E and O forms of the locale; in the "C" locale they are the plain forms.
  // A weekday that is not ok() still has its encoding, which %u and %w write as it is (as both
  // libc++ and libstdc++ do; the names %a and %A need a valid one).
  const bool weekday_number = spec == 'u' || spec == 'w';
  if (weekday_number && !f.weekday_known)
    ::ycxx::detail::chrono_missing("std::format: the value does not contain a weekday");
  if (L && mod != 0 && spec != 'z' && spec != 'S' && !(weekday_number && !f.weekday_ok)) {
    if (spec == 'U' || spec == 'V' || spec == 'W')
      valid_date();
    return o.localized(f, spec, mod);
  }
  switch (spec) {
  case 'a': return weekday_name(false);
  case 'A': return weekday_name(true);
  case 'b':
  case 'h': return month_name(false);
  case 'B': return month_name(true);
  case 'c':
    if (L)
      return o.localized(f, 'c', mod);
    return ::ycxx::detail::chrono_write_specs(o, "%a %b %e %H:%M:%S %Y", nullptr,
                                              ::ycxx::detail::chrono_whole_seconds(f), false);
  case 'C': {
    const long long c = ::ycxx::detail::chrono_floor_div(f.year, 100);
    return o.snum(c, 2);
  }
  case 'd': return o.num(f.day, 2);
  case 'e': return o.num(f.day, 2, ' ');
  case 'D': return ::ycxx::detail::chrono_write_specs(o, "%m/%d/%y", nullptr, f, false);
  case 'F': return ::ycxx::detail::chrono_write_specs(o, "%Y-%m-%d", nullptr, f, false);
  case 'g':
  case 'G': {
    valid_date();
    int y;
    unsigned w;
    ::ycxx::detail::chrono_iso_week(f, y, w);
    if (spec == 'g')
      return o.num(static_cast<unsigned>(y < 0 ? -y : y) % 100, 2);
    return o.snum(y, 4);
  }
  case 'H': return o.num(f.hours, 2);
  case 'I': {
    const unsigned long long h = f.hours % 12;
    return o.num(h == 0 ? 12 : h, 2);
  }
  case 'j':
    if (f.is_duration)
      return o.num(f.hours / 24, 1);
    valid_date();
    return o.num(static_cast<unsigned long long>(f.yday + 1), 3);
  case 'm': return o.num(f.month, 2);
  case 'M': return o.num(f.minutes, 2);
  case 'n': return o.ch('\n');
  case 'p':
    if (L)
      return o.localized(f, 'p', 0);
    return o.ascii(f.hours % 24 < 12 ? "AM" : "PM");
  case 'q': return o.b.append(f.suffix, f.suffix_len);
  case 'Q': return o.ascii(f.count, f.count_len);
  case 'r':
    if (L)
      return o.localized(f, 'r', 0);
    return ::ycxx::detail::chrono_write_specs(o, "%I:%M:%S %p", nullptr, ::ycxx::detail::chrono_whole_seconds(f),
                                              false);
  case 'R':
    return ::ycxx::detail::chrono_write_specs(o, "%H:%M", nullptr, f, false);
  case 'S': return secs(true);
  case 't': return o.ch('\t');
  case 'T':
    ::ycxx::detail::chrono_write_specs(o, "%H:%M", nullptr, f, false);
    o.ch(':');
    return secs(true);
  case 'u': return o.num(f.weekday == 0 ? 7 : f.weekday, 1);
  case 'w': return o.num(f.weekday, 1);
  case 'U':
    valid_date();
    return o.num(static_cast<unsigned>((f.yday + 7 - static_cast<int>(f.weekday)) / 7), 2);
  case 'W':
    valid_date();
    return o.num(static_cast<unsigned>((f.yday + 7 - static_cast<int>((f.weekday + 6) % 7)) / 7), 2);
  case 'V': {
    valid_date();
    int y;
    unsigned w;
    ::ycxx::detail::chrono_iso_week(f, y, w);
    return o.num(w, 2);
  }
  case 'x':
    if (L)
      return o.localized(f, 'x', mod);
    return ::ycxx::detail::chrono_write_specs(o, "%m/%d/%y", nullptr, f, false);
  case 'X':
    if (L)
      return o.localized(f, 'X', mod);
    return ::ycxx::detail::chrono_write_specs(o, "%H:%M:%S", nullptr, ::ycxx::detail::chrono_whole_seconds(f), false);
  case 'y': return o.num(static_cast<unsigned>(f.year < 0 ? -f.year : f.year) % 100, 2);
  case 'Y': return o.snum(f.year, 4);
  case 'z': {
    if (!f.has_offset)
      ::ycxx::detail::chrono_missing("std::format: the value has no time zone offset (%z)");
    const long long m = (f.offset < 0 ? -f.offset : f.offset) / 60;
    o.ch(f.offset < 0 ? '-' : '+');
    o.num(static_cast<unsigned long long>(m / 60), 2);
    if (mod != 0)
      o.ch(':');
    return o.num(static_cast<unsigned long long>(m % 60), 2);
  }
  case 'Z':
    if (!f.has_abbrev)
      ::ycxx::detail::chrono_missing("std::format: the value has no time zone abbreviation (%Z)");
    for (char c : f.abbrev)
      o.b.push_back(static_cast<charT>(static_cast<unsigned char>(c)));
    return;
  case '%': return o.ch('%');
  default: ::ycxx::detail::throw_format_error("std::format: invalid chrono conversion specifier");
  }
}

// The chrono-specs [p, e) (or the NTBS p when e is null); with `sign`, a negative duration gets
// its '-' before the first conversion specifier ([time.format]/4).
template <class charT, class PC>
void chrono_write_specs(chrono_out<charT>& o, const PC* p, std::type_identity_t<const PC*> e,
                        const chrono_fields<charT>& f, bool sign) {
  for (; e == nullptr ? *p != PC(0) : p != e; ++p) {
    if (*p != PC('%')) {
      o.b.push_back(static_cast<charT>(*p));
      continue;
    }
    ++p;
    char mod = 0;
    if (*p == PC('E') || *p == PC('O'))
      mod = static_cast<char>(*p++);
    if (sign) {
      o.ch('-');
      sign = false;
    }
    ::ycxx::detail::chrono_write_one(o, f, static_cast<char>(*p), mod);
  }
}

// ---- chrono-format-spec ------------------------------------------------------------------------

template <class charT>
struct chrono_spec {
  fmt_spec<charT> std;
  const charT* first = nullptr; // the chrono-specs
  const charT* last = nullptr;
};

// What a specifier needs (0: nothing), or ~0u for an invalid one.
constexpr unsigned chrono_needs(char spec, char mod) noexcept {
  constexpr const char* e_ok = "cCxXyYz";
  constexpr const char* o_ok = "deHImMSuUVwWyz";
  if (mod != 0) {
    bool ok = false;
    for (const char* q = mod == 'E' ? e_ok : o_ok; *q != 0; ++q)
      ok = ok || *q == spec;
    if (!ok)
      return ~0u;
  }
  switch (spec) {
  case 'a': case 'A': case 'u': case 'w': return ci_weekday;
  case 'b': case 'B': case 'h': case 'm': return ci_month;
  case 'C': case 'y': case 'Y': return ci_year;
  case 'd': case 'e': return ci_day;
  case 'D': case 'F': case 'x': return ci_year | ci_month | ci_day;
  case 'g': case 'G': case 'U': case 'V': case 'W': return ci_date;
  case 'j': return ci_date; // or ci_duration, see below
  case 'c': return ci_date | ci_time;
  case 'H': case 'I': case 'M': case 'S': case 'p': case 'r': case 'R': case 'T': case 'X': return ci_time;
  case 'q': case 'Q': return ci_duration;
  case 'z': return ci_offset;
  case 'Z': return ci_zone;
  case 'n': case 't': case '%': return 0;
  default: return ~0u;
  }
}

// Parses a chrono-format-spec for a type with `info`; `precision_ok` for a floating-point duration.
template <class charT>
constexpr const charT* chrono_parse_spec(std::basic_format_parse_context<charT>& pc, chrono_spec<charT>& s,
                                         unsigned info, bool precision_ok) {
  const charT* p = pc.begin();
  const charT* const e = pc.end();
  p = ::ycxx::detail::fmt_parse_fill_align(p, e, s.std);
  p = ::ycxx::detail::fmt_parse_width(pc, p, e, s.std);
  if (p != e && *p == charT('.')) {
    ++p;
    if (p != e && ::ycxx::detail::fmt_is_digit(*p)) {
      p = ::ycxx::detail::fmt_parse_number(p, e, s.std.precision);
      s.std.prec_kind = fmt_dyn::value;
    } else if (p != e && *p == charT('{')) {
      p = ::ycxx::detail::fmt_parse_dynamic(pc, p + 1, e, s.std.precision);
      s.std.prec_kind = fmt_dyn::arg;
    } else {
      ::ycxx::detail::throw_format_error("std::format: missing precision after '.'");
    }
    if (!precision_ok)
      ::ycxx::detail::throw_format_error(
          "std::format: a precision is valid only for durations with a floating-point representation");
  }
  if (p != e && *p == charT('L'))
    s.std.localized = true, ++p;
  s.first = p;
  if (p != e && *p != charT('}') && *p != charT('%'))
    ::ycxx::detail::throw_format_error("std::format: chrono-specs must begin with a conversion specifier");
  while (p != e && *p != charT('}')) {
    if (*p == charT('{'))
      ::ycxx::detail::throw_format_error("std::format: '{' in chrono-specs");
    if (*p != charT('%')) {
      ++p;
      continue;
    }
    ++p;
    char mod = 0;
    if (p != e && (*p == charT('E') || *p == charT('O')))
      mod = static_cast<char>(*p++);
    if (p == e || static_cast<unsigned>(*p) > 127u)
      ::ycxx::detail::throw_format_error("std::format: incomplete chrono conversion specifier");
    const char spec = static_cast<char>(*p++);
    unsigned need = ::ycxx::detail::chrono_needs(spec, mod);
    if (need == ~0u)
      ::ycxx::detail::throw_format_error("std::format: invalid chrono conversion specifier");
    if (spec == 'j' && (info & ci_duration) != 0)
      need = ci_duration;
    if ((need & info) != need)
      ::ycxx::detail::throw_format_error(
          "std::format: the formatted type does not contain the information a conversion specifier refers to");
  }
  s.last = p;
  return p;
}

// Pads the text in b as the std-format-spec asks.
template <class charT, class Context>
typename Context::iterator chrono_emit(const chrono_spec<charT>& s, Context& ctx, const fmt_dynbuf<charT>& b) {
  const std::size_t width = ::ycxx::detail::fmt_width(s.std, ctx);
  if (width == 0)
    return ::ycxx::detail::fmt_put<charT>(ctx.out(), b.data(), b.size());
  return ::ycxx::detail::fmt_write_padded<charT>(ctx.out(), s.std, fmt_align::left, width,
                                                 uni::width(b.data(), b.size()), b.data(), b.size());
}

// ---- the representations without chrono-specs (the stream inserters') ---------------------

template <class charT, class Rep, class Period>
void chrono_default(chrono_out<charT>& o, const std::chrono::duration<Rep, Period>& d, long long precision = -1) {
  // As `ostringstream s; s << d.count() << units-suffix` with the formatting locale.
  char buf[128];
  std::to_chars_result r;
  const Rep c = d.count();
  if constexpr (std::chrono::treat_as_floating_point_v<Rep>)
    r = std::to_chars(buf, buf + sizeof buf, c, std::chars_format::general,
                      precision < 0 ? 6 : static_cast<int>(precision < 100 ? precision : 100));
  else if constexpr (std::is_signed_v<Rep>)
    r = std::to_chars(buf, buf + sizeof buf, static_cast<long long>(c));
  else
    r = std::to_chars(buf, buf + sizeof buf, static_cast<unsigned long long>(c));
  o.count(buf, r.ptr);
  const chrono_suffix_text<charT> s = ::ycxx::detail::chrono_suffix<Period, charT>();
  o.b.append(s.text, s.len);
}

template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::day& d) {
  chrono_fields<charT> f;
  f.day = static_cast<unsigned>(d);
  o.num(f.day, 2);
  if (!d.ok())
    o.ascii(" is not a valid day");
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::month& m) {
  if (m.ok()) {
    chrono_fields<charT> f;
    f.month = static_cast<unsigned>(m);
    return ::ycxx::detail::chrono_write_one(o, f, 'b', 0);
  }
  o.num(static_cast<unsigned>(m), 1);
  o.ascii(" is not a valid month");
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::year& y) {
  o.snum(static_cast<int>(y), 4);
  if (!y.ok())
    o.ascii(" is not a valid year");
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::weekday& wd) {
  if (wd.ok()) {
    chrono_fields<charT> f;
    f.weekday = wd.c_encoding();
    f.weekday_ok = f.weekday_known = true;
    return ::ycxx::detail::chrono_write_one(o, f, 'a', 0);
  }
  o.num(wd.c_encoding(), 1);
  o.ascii(" is not a valid weekday");
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::weekday_indexed& wdi) {
  ::ycxx::detail::chrono_default(o, wdi.weekday());
  o.ch('[');
  o.num(wdi.index(), 1);
  if (wdi.index() < 1 || wdi.index() > 5)
    o.ascii(" is not a valid index");
  o.ch(']');
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::weekday_last& wdl) {
  ::ycxx::detail::chrono_default(o, wdl.weekday());
  o.ascii("[last]");
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::month_day& md) {
  ::ycxx::detail::chrono_default(o, md.month());
  o.ch('/');
  ::ycxx::detail::chrono_default(o, md.day());
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::month_day_last& mdl) {
  ::ycxx::detail::chrono_default(o, mdl.month());
  o.ascii("/last");
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::month_weekday& mwd) {
  ::ycxx::detail::chrono_default(o, mwd.month());
  o.ch('/');
  ::ycxx::detail::chrono_default(o, mwd.weekday_indexed());
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::month_weekday_last& mwdl) {
  ::ycxx::detail::chrono_default(o, mwdl.month());
  o.ch('/');
  ::ycxx::detail::chrono_default(o, mwdl.weekday_last());
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::year_month& ym) {
  ::ycxx::detail::chrono_default(o, ym.year());
  o.ch('/');
  ::ycxx::detail::chrono_default(o, ym.month());
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::year_month_day& ymd) {
  chrono_fields<charT> f;
  ::ycxx::detail::chrono_set_ymd(f, ymd);
  ::ycxx::detail::chrono_write_specs(o, "%Y-%m-%d", nullptr, f, false);
  if (!ymd.ok())
    o.ascii(" is not a valid date");
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::year_month_day_last& ymdl) {
  ::ycxx::detail::chrono_default(o, ymdl.year());
  o.ch('/');
  ::ycxx::detail::chrono_default(o, ymdl.month_day_last());
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::year_month_weekday& ymwd) {
  ::ycxx::detail::chrono_default(o, ymwd.year());
  o.ch('/');
  ::ycxx::detail::chrono_default(o, ymwd.month());
  o.ch('/');
  ::ycxx::detail::chrono_default(o, ymwd.weekday_indexed());
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::year_month_weekday_last& ymwdl) {
  ::ycxx::detail::chrono_default(o, ymwdl.year());
  o.ch('/');
  ::ycxx::detail::chrono_default(o, ymwdl.month());
  o.ch('/');
  ::ycxx::detail::chrono_default(o, ymwdl.weekday_last());
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::sys_info& i) {
  chrono_fields<charT> f;
  o.ch('[');
  ::ycxx::detail::chrono_set_point(f, i.begin.time_since_epoch());
  ::ycxx::detail::chrono_write_specs(o, "%F %T", nullptr, f, false);
  o.ascii(", ");
  f = chrono_fields<charT>();
  ::ycxx::detail::chrono_set_point(f, i.end.time_since_epoch());
  ::ycxx::detail::chrono_write_specs(o, "%F %T", nullptr, f, false);
  o.ascii(") ");
  f = chrono_fields<charT>();
  ::ycxx::detail::chrono_set_time(f, i.offset);
  ::ycxx::detail::chrono_write_specs(o, "%T", nullptr, f, f.negative);
  o.ch(' ');
  o.snum(i.save.count(), 1);
  o.ascii("min \"");
  for (char c : i.abbrev)
    o.b.push_back(static_cast<charT>(static_cast<unsigned char>(c)));
  o.ch('"');
}
template <class charT>
void chrono_default(chrono_out<charT>& o, const std::chrono::local_info& i) {
  switch (i.result) {
  case std::chrono::local_info::unique: o.ascii("unique: "); break;
  case std::chrono::local_info::nonexistent: o.ascii("nonexistent: "); break;
  case std::chrono::local_info::ambiguous: o.ascii("ambiguous: "); break;
  default:
    o.ascii("unspecified result (");
    o.snum(i.result, 1);
    o.ascii("): ");
  }
  o.ch('{');
  ::ycxx::detail::chrono_default(o, i.first);
  o.ascii(", ");
  ::ycxx::detail::chrono_default(o, i.second);
  o.ch('}');
}

// The text of a value with chrono-specs (or with the default ones when there are none).
template <class charT, class PC>
void chrono_render(chrono_out<charT>& o, const chrono_fields<charT>& f, const PC* first, const PC* last,
                   const char* dflt) {
  if (first != last)
    ::ycxx::detail::chrono_write_specs(o, first, last, f, f.negative);
  else
    ::ycxx::detail::chrono_write_specs(o, dflt, nullptr, f, f.negative);
}

// The fields of each formattable type and the information it has.
template <class charT, class Rep, class Period>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::duration<Rep, Period>& d) {
  chrono_fields<charT> f;
  ::ycxx::detail::chrono_set_duration(f, d);
  return f;
}
template <class charT, class Duration>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::sys_time<Duration>& t) {
  chrono_fields<charT> f;
  ::ycxx::detail::chrono_set_point(f, t.time_since_epoch());
  f.has_abbrev = f.has_offset = true;
  f.abbrev = "UTC";
  return f;
}
template <class charT, class Duration>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::local_time<Duration>& t) {
  chrono_fields<charT> f;
  ::ycxx::detail::chrono_set_point(f, t.time_since_epoch());
  return f;
}
template <class charT, class Duration>
chrono_fields<charT> chrono_fields_of(const std::chrono::utc_time<Duration>& t) {
  using cd = std::common_type_t<Duration, std::chrono::seconds>;
  const std::chrono::leap_second_info lsi = std::chrono::get_leap_second_info(t);
  chrono_fields<charT> f;
  ::ycxx::detail::chrono_set_point(f, cd(t.time_since_epoch()) - lsi.elapsed);
  if (lsi.is_leap_second)
    f.seconds = 60; // [time.format]/11: during a leap second the seconds are 60
  f.has_abbrev = f.has_offset = true;
  f.abbrev = "UTC";
  return f;
}
template <class charT, class Duration>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::tai_time<Duration>& t) {
  // [time.format]/12: the sys_time 1958-01-01 is the TAI epoch (4383 days before 1970).
  chrono_fields<charT> f;
  ::ycxx::detail::chrono_set_point(f, t.time_since_epoch(), -4383);
  f.has_abbrev = f.has_offset = true;
  f.abbrev = "TAI";
  return f;
}
template <class charT, class Duration>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::gps_time<Duration>& t) {
  // [time.format]/13: the GPS epoch is 1980-01-06, 3657 days after 1970-01-01.
  chrono_fields<charT> f;
  ::ycxx::detail::chrono_set_point(f, t.time_since_epoch(), 3657);
  f.has_abbrev = f.has_offset = true;
  f.abbrev = "GPS";
  return f;
}
template <class charT, class Duration>
chrono_fields<charT> chrono_fields_of(const std::chrono::file_time<Duration>& t) {
  return ::ycxx::detail::chrono_fields_of<charT>(std::chrono::clock_cast<std::chrono::system_clock>(t));
}
template <class charT, class Duration>
constexpr chrono_fields<charT> chrono_fields_of(const ycxx::adl_free::local_time_format_t<Duration>& t) {
  chrono_fields<charT> f;
  ::ycxx::detail::chrono_set_point(f, t.time_.time_since_epoch());
  if (t.abbrev_ != nullptr) {
    f.has_abbrev = true;
    f.abbrev = *t.abbrev_;
  }
  if (t.offset_sec_ != nullptr) {
    f.has_offset = true;
    f.offset = t.offset_sec_->count();
  }
  return f;
}
template <class charT, class Duration>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::hh_mm_ss<Duration>& h) {
  chrono_fields<charT> f;
  ::ycxx::detail::chrono_set_time(f, h.to_duration());
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::day& d) {
  chrono_fields<charT> f;
  f.day = static_cast<unsigned>(d);
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::month& m) {
  chrono_fields<charT> f;
  f.month = static_cast<unsigned>(m);
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::year& y) {
  chrono_fields<charT> f;
  f.year = static_cast<int>(y);
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::weekday& wd) {
  chrono_fields<charT> f;
  f.weekday = wd.c_encoding();
  f.weekday_ok = wd.ok();
  f.weekday_known = true;
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::weekday_indexed& wdi) {
  return ::ycxx::detail::chrono_fields_of<charT>(wdi.weekday());
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::weekday_last& wdl) {
  return ::ycxx::detail::chrono_fields_of<charT>(wdl.weekday());
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::month_day& md) {
  chrono_fields<charT> f;
  f.month = static_cast<unsigned>(md.month());
  f.day = static_cast<unsigned>(md.day());
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::month_day_last& mdl) {
  chrono_fields<charT> f;
  f.month = static_cast<unsigned>(mdl.month());
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::month_weekday& mwd) {
  chrono_fields<charT> f = ::ycxx::detail::chrono_fields_of<charT>(mwd.weekday_indexed().weekday());
  f.month = static_cast<unsigned>(mwd.month());
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::month_weekday_last& mwdl) {
  chrono_fields<charT> f = ::ycxx::detail::chrono_fields_of<charT>(mwdl.weekday_last().weekday());
  f.month = static_cast<unsigned>(mwdl.month());
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::year_month& ym) {
  chrono_fields<charT> f;
  f.year = static_cast<int>(ym.year());
  f.month = static_cast<unsigned>(ym.month());
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::year_month_day& ymd) {
  chrono_fields<charT> f;
  ::ycxx::detail::chrono_set_ymd(f, ymd);
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::year_month_day_last& ymdl) {
  return ::ycxx::detail::chrono_fields_of<charT>(std::chrono::year_month_day(ymdl));
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::year_month_weekday& ymwd) {
  chrono_fields<charT> f;
  if (ymwd.ok()) {
    ::ycxx::detail::chrono_set_days(f, std::chrono::sys_days(ymwd).time_since_epoch().count());
  } else {
    f.year = static_cast<int>(ymwd.year());
    f.month = static_cast<unsigned>(ymwd.month());
    f.weekday = ymwd.weekday().c_encoding();
    f.weekday_ok = ymwd.weekday().ok();
    f.weekday_known = true;
  }
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::year_month_weekday_last& ymwdl) {
  chrono_fields<charT> f;
  if (ymwdl.ok()) {
    ::ycxx::detail::chrono_set_days(f, std::chrono::sys_days(ymwdl).time_since_epoch().count());
  } else {
    f.year = static_cast<int>(ymwdl.year());
    f.month = static_cast<unsigned>(ymwdl.month());
    f.weekday = ymwdl.weekday().c_encoding();
    f.weekday_ok = ymwdl.weekday().ok();
    f.weekday_known = true;
  }
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::sys_info& i) {
  chrono_fields<charT> f;
  f.has_abbrev = f.has_offset = true;
  f.abbrev = i.abbrev;
  f.offset = i.offset.count();
  return f;
}
template <class charT>
constexpr chrono_fields<charT> chrono_fields_of(const std::chrono::local_info&) {
  return chrono_fields<charT>();
}

// The information of each type, and its default chrono-specs (null: a chrono_default overload).
template <class T>
struct chrono_traits {
  static constexpr unsigned info = ci_full_date | ci_time;
  static constexpr const char* dflt = "%F %T";
};
template <class Rep, class Period>
struct chrono_traits<std::chrono::duration<Rep, Period>> {
  static constexpr unsigned info = ci_time | ci_duration;
  static constexpr const char* dflt = nullptr;
};
template <class Duration>
struct chrono_traits<std::chrono::sys_time<Duration>> {
  static constexpr unsigned info = ci_full_date | ci_time | ci_zone | ci_offset;
  // os << sys_days is os << year_month_day{dp}. (Duration{1} < days{1}, compared as ratios: in
  // the common type a day of a fine period such as femto overflows.)
  static constexpr const char* dflt = std::chrono::treat_as_floating_point_v<typename Duration::rep> ||
                                              std::ratio_less_v<typename Duration::period, std::ratio<86400>>
                                          ? "%F %T"
                                          : "%F";
};
template <class Duration>
struct chrono_traits<std::chrono::local_time<Duration>> {
  static constexpr unsigned info = ci_full_date | ci_time;
  static constexpr const char* dflt = chrono_traits<std::chrono::sys_time<Duration>>::dflt;
};
template <class Duration>
struct chrono_traits<std::chrono::utc_time<Duration>> : chrono_traits<std::chrono::sys_time<Duration>> {
  static constexpr const char* dflt = "%F %T";
};
template <class Duration>
struct chrono_traits<std::chrono::tai_time<Duration>> : chrono_traits<std::chrono::utc_time<Duration>> {};
template <class Duration>
struct chrono_traits<std::chrono::gps_time<Duration>> : chrono_traits<std::chrono::utc_time<Duration>> {};
template <class Duration>
struct chrono_traits<std::chrono::file_time<Duration>> : chrono_traits<std::chrono::utc_time<Duration>> {};
template <class Duration>
struct chrono_traits<ycxx::adl_free::local_time_format_t<Duration>> {
  static constexpr unsigned info = ci_full_date | ci_time | ci_zone | ci_offset;
  static constexpr const char* dflt = "%F %T %Z";
};
template <class Duration>
struct chrono_traits<std::chrono::hh_mm_ss<Duration>> {
  static constexpr unsigned info = ci_time;
  static constexpr const char* dflt = "%T";
};
template <>
struct chrono_traits<std::chrono::day> {
  static constexpr unsigned info = ci_day;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::month> {
  static constexpr unsigned info = ci_month;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::year> {
  static constexpr unsigned info = ci_year;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::weekday> {
  static constexpr unsigned info = ci_weekday;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::weekday_indexed> : chrono_traits<std::chrono::weekday> {};
template <>
struct chrono_traits<std::chrono::weekday_last> : chrono_traits<std::chrono::weekday> {};
template <>
struct chrono_traits<std::chrono::month_day> {
  static constexpr unsigned info = ci_month | ci_day;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::month_day_last> {
  static constexpr unsigned info = ci_month;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::month_weekday> {
  static constexpr unsigned info = ci_month | ci_weekday;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::month_weekday_last> : chrono_traits<std::chrono::month_weekday> {};
template <>
struct chrono_traits<std::chrono::year_month> {
  static constexpr unsigned info = ci_year | ci_month;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::year_month_day> {
  static constexpr unsigned info = ci_full_date;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::year_month_day_last> {
  static constexpr unsigned info = ci_full_date;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::year_month_weekday> : chrono_traits<std::chrono::year_month_day_last> {};
template <>
struct chrono_traits<std::chrono::year_month_weekday_last> : chrono_traits<std::chrono::year_month_day_last> {};
template <>
struct chrono_traits<std::chrono::sys_info> {
  static constexpr unsigned info = ci_zone | ci_offset;
  static constexpr const char* dflt = nullptr;
};
template <>
struct chrono_traits<std::chrono::local_info> {
  static constexpr unsigned info = 0;
  static constexpr const char* dflt = nullptr;
};

template <class T>
inline constexpr bool chrono_float_duration = false;
template <class Rep, class Period>
inline constexpr bool chrono_float_duration<std::chrono::duration<Rep, Period>> =
    std::chrono::treat_as_floating_point_v<Rep>;

// Writes v into b with the chrono-specs [first, last) (none: the default representation).
template <class charT, class T, class PC>
void chrono_text(fmt_dynbuf<charT>& b, const T& v, const PC* first, const PC* last, const std::locale* loc,
                 long long precision = -1) {
  chrono_out<charT> o{b, loc};
  if (first == last && chrono_traits<T>::dflt == nullptr) {
    if constexpr (requires { ::ycxx::detail::chrono_default(o, v); }) {
      if constexpr (requires { typename T::period; })
        ::ycxx::detail::chrono_default(o, v, precision);
      else
        ::ycxx::detail::chrono_default(o, v);
    }
    return;
  }
  const chrono_fields<charT> f = ::ycxx::detail::chrono_fields_of<charT>(v);
  ::ycxx::detail::chrono_render(o, f, first, last, chrono_traits<T>::dflt);
}

// The common formatter: parse() checks the chrono-format-spec against T's information.
template <class T, class charT>
struct chrono_formatter {
  chrono_spec<charT> spec_;

  constexpr typename std::basic_format_parse_context<charT>::iterator
  parse(std::basic_format_parse_context<charT>& pc) {
    return ::ycxx::detail::chrono_parse_spec(pc, spec_, chrono_traits<T>::info, chrono_float_duration<T>);
  }
  template <class FormatContext>
  typename FormatContext::iterator format(const T& v, FormatContext& ctx) const {
    fmt_dynbuf<charT> b;
    const long long precision = ::ycxx::detail::fmt_precision(spec_.std, ctx);
    if (spec_.std.localized) {
      const std::locale loc = ctx.locale();
      ::ycxx::detail::chrono_text(b, v, spec_.first, spec_.last, __builtin_addressof(loc), precision);
    } else {
      ::ycxx::detail::chrono_text(b, v, spec_.first, spec_.last, nullptr, precision);
    }
    return ::ycxx::detail::chrono_emit(spec_, ctx, b);
  }
};

// The stream inserters: the default representation with the stream's locale.
template <class charT, class traits, class T>
std::basic_ostream<charT, traits>& chrono_insert(std::basic_ostream<charT, traits>& os, const T& v,
                                                 const char* specs = nullptr) {
  fmt_dynbuf<charT> b;
  const std::locale loc = os.getloc();
  if (specs == nullptr)
    ::ycxx::detail::chrono_text(b, v, specs, specs, __builtin_addressof(loc));
  else
    ::ycxx::detail::chrono_text(b, v, specs, specs + std::char_traits<char>::length(specs), __builtin_addressof(loc));
  return os << std::basic_string_view<charT, traits>(b.data(), b.size());
}

// Formats as the stream inserter would with the "C" locale, into a std::string (the exception
// messages of [time.zone.exception]).
template <class T>
void chrono_append(std::string& s, const T& v) {
  fmt_dynbuf<char> b;
  ::ycxx::detail::chrono_text(b, v, static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), nullptr);
  s.append(b.data(), b.size());
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [time.format]: the formatters.
template <class Rep, class Period, ycxx::detail::fmt_char charT>
struct formatter<chrono::duration<Rep, Period>, charT>
    : ycxx::detail::chrono_formatter<chrono::duration<Rep, Period>, charT> {};
template <class Duration, ycxx::detail::fmt_char charT>
struct formatter<chrono::sys_time<Duration>, charT>
    : ycxx::detail::chrono_formatter<chrono::sys_time<Duration>, charT> {};
template <class Duration, ycxx::detail::fmt_char charT>
struct formatter<chrono::utc_time<Duration>, charT>
    : ycxx::detail::chrono_formatter<chrono::utc_time<Duration>, charT> {};
template <class Duration, ycxx::detail::fmt_char charT>
struct formatter<chrono::tai_time<Duration>, charT>
    : ycxx::detail::chrono_formatter<chrono::tai_time<Duration>, charT> {};
template <class Duration, ycxx::detail::fmt_char charT>
struct formatter<chrono::gps_time<Duration>, charT>
    : ycxx::detail::chrono_formatter<chrono::gps_time<Duration>, charT> {};
template <class Duration, ycxx::detail::fmt_char charT>
struct formatter<chrono::file_time<Duration>, charT>
    : ycxx::detail::chrono_formatter<chrono::file_time<Duration>, charT> {};
template <class Duration, ycxx::detail::fmt_char charT>
struct formatter<chrono::local_time<Duration>, charT>
    : ycxx::detail::chrono_formatter<chrono::local_time<Duration>, charT> {};
template <class Duration, ycxx::detail::fmt_char charT>
struct formatter<ycxx::adl_free::local_time_format_t<Duration>, charT>
    : ycxx::detail::chrono_formatter<ycxx::adl_free::local_time_format_t<Duration>, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::day, charT> : ycxx::detail::chrono_formatter<chrono::day, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::month, charT> : ycxx::detail::chrono_formatter<chrono::month, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::year, charT> : ycxx::detail::chrono_formatter<chrono::year, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::weekday, charT> : ycxx::detail::chrono_formatter<chrono::weekday, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::weekday_indexed, charT> : ycxx::detail::chrono_formatter<chrono::weekday_indexed, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::weekday_last, charT> : ycxx::detail::chrono_formatter<chrono::weekday_last, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::month_day, charT> : ycxx::detail::chrono_formatter<chrono::month_day, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::month_day_last, charT> : ycxx::detail::chrono_formatter<chrono::month_day_last, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::month_weekday, charT> : ycxx::detail::chrono_formatter<chrono::month_weekday, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::month_weekday_last, charT>
    : ycxx::detail::chrono_formatter<chrono::month_weekday_last, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::year_month, charT> : ycxx::detail::chrono_formatter<chrono::year_month, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::year_month_day, charT> : ycxx::detail::chrono_formatter<chrono::year_month_day, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::year_month_day_last, charT>
    : ycxx::detail::chrono_formatter<chrono::year_month_day_last, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::year_month_weekday, charT>
    : ycxx::detail::chrono_formatter<chrono::year_month_weekday, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::year_month_weekday_last, charT>
    : ycxx::detail::chrono_formatter<chrono::year_month_weekday_last, charT> {};
template <class Rep, class Period, ycxx::detail::fmt_char charT>
struct formatter<chrono::hh_mm_ss<chrono::duration<Rep, Period>>, charT>
    : ycxx::detail::chrono_formatter<chrono::hh_mm_ss<chrono::duration<Rep, Period>>, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::sys_info, charT> : ycxx::detail::chrono_formatter<chrono::sys_info, charT> {};
template <ycxx::detail::fmt_char charT>
struct formatter<chrono::local_info, charT> : ycxx::detail::chrono_formatter<chrono::local_info, charT> {};

// [time.format]/19
template <class Duration, class TimeZonePtr, ycxx::detail::fmt_char charT>
struct formatter<chrono::zoned_time<Duration, TimeZonePtr>, charT>
    : formatter<ycxx::adl_free::local_time_format_t<common_type_t<Duration, chrono::seconds>>, charT> {
  template <class FormatContext>
  typename FormatContext::iterator format(const chrono::zoned_time<Duration, TimeZonePtr>& tp,
                                          FormatContext& ctx) const {
    const chrono::sys_info info = tp.get_info();
    return formatter<ycxx::adl_free::local_time_format_t<common_type_t<Duration, chrono::seconds>>, charT>::format(
        {tp.get_local_time(), __builtin_addressof(info.abbrev), __builtin_addressof(info.offset)}, ctx);
  }
};

// [format.formatter.spec]/3 and [time.format]/8-9.
template <class Rep, class Period>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::duration<Rep, Period>> =
    enable_nonlocking_formatter_optimization<Rep>;
template <class Duration>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::zoned_time<Duration, const chrono::time_zone*>> =
    true;
template <class Clock, class Duration>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::time_point<Clock, Duration>> = true;
template <class Duration>
inline constexpr bool enable_nonlocking_formatter_optimization<ycxx::adl_free::local_time_format_t<Duration>> = true;
template <class Duration>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::hh_mm_ss<Duration>> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::day> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::month> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::year> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::weekday> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::weekday_indexed> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::weekday_last> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::month_day> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::month_day_last> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::month_weekday> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::month_weekday_last> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::year_month> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::year_month_day> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::year_month_day_last> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::year_month_weekday> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::year_month_weekday_last> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::sys_info> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::local_info> = true;

} // namespace std

namespace [[gnu::visibility("hidden")]] std { namespace chrono {

// [time.duration.io]/1
template <class charT, class traits, class Rep, class Period>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const duration<Rep, Period>& d) {
  // s.flags(os.flags()); s.imbue(os.getloc()); s.precision(os.precision()); s << d.count() << suffix,
  // through a stream on a string buffer of its own (no <sstream> needed).
  struct string_buf : basic_streambuf<charT, traits> {
    basic_string<charT, traits> text;
    typename traits::int_type overflow(typename traits::int_type c) override {
      if (!traits::eq_int_type(c, traits::eof()))
        text.push_back(traits::to_char_type(c));
      return traits::not_eof(c);
    }
    ptrdiff_t xsputn(const charT* p, ptrdiff_t n) override { // streamsize
      text.append(p, static_cast<size_t>(n));
      return n;
    }
  } buf;
  basic_ostream<charT, traits> s(__builtin_addressof(buf));
  s.flags(os.flags());
  s.imbue(os.getloc());
  s.precision(os.precision());
  s << d.count();
  const ycxx::detail::chrono_suffix_text<charT> suffix = ycxx::detail::chrono_suffix<Period, charT>();
  buf.text.append(suffix.text, suffix.len);
  return os << buf.text;
}

// [time.clock.system.nonmembers]/1-5
template <class charT, class traits, class Duration>
  requires(!treat_as_floating_point_v<typename Duration::rep> && Duration(1) < days(1))
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const sys_time<Duration>& tp) {
  return ycxx::detail::chrono_insert(os, tp, "%F %T");
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const sys_days& dp) {
  return ycxx::detail::chrono_insert(os, year_month_day(dp));
}
template <class charT, class traits, class Duration>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const utc_time<Duration>& t) {
  return ycxx::detail::chrono_insert(os, t, "%F %T");
}
template <class charT, class traits, class Duration>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const tai_time<Duration>& t) {
  return ycxx::detail::chrono_insert(os, t, "%F %T");
}
template <class charT, class traits, class Duration>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const gps_time<Duration>& t) {
  return ycxx::detail::chrono_insert(os, t, "%F %T");
}
template <class charT, class traits, class Duration>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const file_time<Duration>& t) {
  return ycxx::detail::chrono_insert(os, t, "%F %T");
}
// [time.clock.local]/2-4
template <class charT, class traits, class Duration>
  requires requires(basic_ostream<charT, traits>& os, const sys_time<Duration>& st) { os << st; }
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const local_time<Duration>& lt) {
  return os << sys_time<Duration>(lt.time_since_epoch());
}

// [time.cal]
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const day& d) {
  return ycxx::detail::chrono_insert(os, d);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const month& m) {
  return ycxx::detail::chrono_insert(os, m);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const year& y) {
  return ycxx::detail::chrono_insert(os, y);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const weekday& wd) {
  return ycxx::detail::chrono_insert(os, wd);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const weekday_indexed& wdi) {
  return ycxx::detail::chrono_insert(os, wdi);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const weekday_last& wdl) {
  return ycxx::detail::chrono_insert(os, wdl);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const month_day& md) {
  return ycxx::detail::chrono_insert(os, md);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const month_day_last& mdl) {
  return ycxx::detail::chrono_insert(os, mdl);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const month_weekday& mwd) {
  return ycxx::detail::chrono_insert(os, mwd);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const month_weekday_last& mwdl) {
  return ycxx::detail::chrono_insert(os, mwdl);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const year_month& ym) {
  return ycxx::detail::chrono_insert(os, ym);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const year_month_day& ymd) {
  return ycxx::detail::chrono_insert(os, ymd);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const year_month_day_last& ymdl) {
  return ycxx::detail::chrono_insert(os, ymdl);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const year_month_weekday& ymwd) {
  return ycxx::detail::chrono_insert(os, ymwd);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const year_month_weekday_last& ymwdl) {
  return ycxx::detail::chrono_insert(os, ymwdl);
}

// [time.hms.nonmembers]
template <class charT, class traits, class Duration>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const hh_mm_ss<Duration>& hms) {
  return ycxx::detail::chrono_insert(os, hms, "%T");
}

// [time.zone.info]: unspecified formats.
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const sys_info& r) {
  return ycxx::detail::chrono_insert(os, r);
}
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const local_info& r) {
  return ycxx::detail::chrono_insert(os, r);
}

// [time.zone.zonedtime.nonmembers]/2
template <class charT, class traits, class Duration, class TimeZonePtr>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const zoned_time<Duration, TimeZonePtr>& t) {
  const sys_info info = t.get_info();
  return ycxx::detail::chrono_insert(
      os, local_time_format(t.get_local_time(), __builtin_addressof(info.abbrev), __builtin_addressof(info.offset)),
      "%F %T %Z");
}

// [time.zone.exception]
template <class Duration>
nonexistent_local_time::nonexistent_local_time(const local_time<Duration>& tp, const local_info& i)
    : runtime_error([&] {
        string s;
        ycxx::detail::chrono_append(s, tp);
        s += " is in a gap between\n";
        ycxx::detail::chrono_append(s, local_seconds(i.first.end.time_since_epoch()) + i.first.offset);
        s += ' ';
        s += i.first.abbrev;
        s += " and\n";
        ycxx::detail::chrono_append(s, local_seconds(i.second.begin.time_since_epoch()) + i.second.offset);
        s += ' ';
        s += i.second.abbrev;
        s += " which are both equivalent to\n";
        ycxx::detail::chrono_append(s, i.first.end);
        s += " UTC";
        return s;
      }()) {}

template <class Duration>
ambiguous_local_time::ambiguous_local_time(const local_time<Duration>& tp, const local_info& i)
    : runtime_error([&] {
        string s;
        ycxx::detail::chrono_append(s, tp);
        s += " is ambiguous.  It could be\n";
        ycxx::detail::chrono_append(s, tp);
        s += ' ';
        s += i.first.abbrev;
        s += " == ";
        ycxx::detail::chrono_append(s, tp - i.first.offset);
        s += " UTC or\n";
        ycxx::detail::chrono_append(s, tp);
        s += ' ';
        s += i.second.abbrev;
        s += " == ";
        ycxx::detail::chrono_append(s, tp - i.second.offset);
        s += " UTC";
        return s;
      }()) {}

}} // namespace std::chrono
