// libycxx hosted: the text forms of <chrono> values: the formatters ([time.format]), the stream
// inserters of every chrono type, local_time_format, and the constructors of the time zone
// exception classes ([time.zone.exception]). Parsing is in ycxx/hosted/chrono_parse.hpp.
//
// Every formatter turns its value into one set of fields (date, time of day, zone) and runs the
// chrono-specs over them into a local buffer, which is then padded as a whole. Without the L
// option the "C" locale's names and decimal point are built in; with it, the locale-dependent
// conversions (%a %A %b %B %c %p %r %x %X and the E/O-modified ones) go through the formatting
// locale's time_put facet (a call into the hosted runtime, src/hosted/chrono.cpp) unless that is
// the classic locale's facet, %S takes the locale's decimal point and the counts of durations
// its num_put.
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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// [time.format]: local-time-format-t.
template <class _Duration>
struct __local_time_format_t {
  std::chrono::local_time<_Duration> __time_;
  const std::string* __abbrev_;
  const std::chrono::seconds* __offset_sec_;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {
template <class _Duration>
__ycxx::__adl_free::__local_time_format_t<_Duration> local_time_format(local_time<_Duration> time,
                                                                const string* abbrev = nullptr,
                                                                const seconds* __offset_sec = nullptr) {
  return {time, abbrev, __offset_sec};
}
}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// ---- the fields of a value ------------------------------------------------------------------

// The information a type provides ([time.format]/3, /6).
enum __chrono_info : unsigned {
  __ci_year = 1,
  __ci_month = 2,
  __ci_day = 4,
  __ci_weekday = 8,
  __ci_date = 16, // a complete date: day of the year, week numbers
  __ci_time = 32, // a time of day (or a duration read as one)
  __ci_duration = 64,
  __ci_zone = 128,   // %Z
  __ci_offset = 256, // %z
  __ci_full_date = __ci_year | __ci_month | __ci_day | __ci_weekday | __ci_date,
};

template <class __charT>
struct __chrono_fields {
  int year = 0;
  unsigned month = 0, day = 0, weekday = 0;
  int __yday = 0; // 0 for January 1st
  bool __weekday_ok = false;
  bool __date_ok = false; // year, month, day, weekday and yday form a valid date
  bool __negative = false;
  unsigned long long hours = 0;
  unsigned minutes = 0, seconds = 0;
  unsigned long long subseconds = 0;
  unsigned width = 0; // fractional digits of the seconds
  bool __is_duration = false;
  bool __has_abbrev = false, __has_offset = false;
  std::string_view abbrev;
  long long offset = 0; // seconds
  // %Q and %q of a duration
  char count[64] = {};
  std::size_t __count_len = 0;
  __charT suffix[48] = {};
  std::size_t __suffix_len = 0;
};

template <class __charT>
constexpr void __chrono_set_days(__chrono_fields<__charT>& __f, long long __z) noexcept {
  const __civil_date c = ::__ycxx::__detail::__civil_from_days(__z);
  __f.year = c.y;
  __f.month = c.m;
  __f.day = c.d;
  __f.weekday = ::__ycxx::__detail::__weekday_from_days(__z);
  __f.__weekday_ok = __f.__date_ok = true;
  __f.__yday = static_cast<int>(::__ycxx::__detail::__wrap_sub(__z, ::__ycxx::__detail::__days_from_civil(c.y, 1, 1)));
}

// y/m/d as given (not necessarily a valid date); the weekday only for a valid one.
template <class __charT>
constexpr void __chrono_set_ymd(__chrono_fields<__charT>& __f, const std::chrono::year_month_day& ymd) noexcept {
  if (ymd.ok()) {
    ::__ycxx::__detail::__chrono_set_days(__f, std::chrono::sys_days(ymd).time_since_epoch().count());
    return;
  }
  __f.year = static_cast<int>(ymd.year());
  __f.month = static_cast<unsigned>(ymd.month());
  __f.day = static_cast<unsigned>(ymd.day());
  if (__f.month >= 1 && __f.month <= 12)
    __f.__yday = ::__ycxx::__detail::__days_from_civil(__f.year, __f.month, __f.day) - ::__ycxx::__detail::__days_from_civil(__f.year, 1, 1);
}

// ---- splitting a count into units ------------------------------------------------------------
// Without hh_mm_ss and duration_cast, whose ratio arithmetic overflows for fine periods (atto
// to hours) and whose negation overflows for the most negative count: the magnitude of the
// count is split by the period's num and den with 128-bit intermediate products.

// floor(a * b / c) (its low 64 bits) and a * b mod c, for c > 0.
struct __chrono_qr {
  unsigned long long __q, r;
};
template <class _Up = __uint128>
constexpr __chrono_qr __chrono_muldiv(unsigned long long a, unsigned long long b, unsigned long long c) noexcept {
  if constexpr (__cfg::__has_int128) {
    const _Up p = static_cast<_Up>(a) * b;
    return {static_cast<unsigned long long>(p / c), static_cast<unsigned long long>(p % c)};
  } else {
    // The 128-bit product, then restoring division one bit at a time.
    const unsigned long long __a0 = a & 0xffffffffu, __a1 = a >> 32, __b0 = b & 0xffffffffu, __b1 = b >> 32;
    const unsigned long long __p00 = __a0 * __b0, __p01 = __a0 * __b1, __p10 = __a1 * __b0, __p11 = __a1 * __b1;
    const unsigned long long __mid = (__p00 >> 32) + (__p01 & 0xffffffffu) + (__p10 & 0xffffffffu);
    const unsigned long long __hi = __p11 + (__p01 >> 32) + (__p10 >> 32) + (__mid >> 32), __lo = (__mid << 32) | (__p00 & 0xffffffffu);
    unsigned long long __q = 0, r = 0;
    for (int i = 127; i >= 0; --i) {
      const bool __carry = (r >> 63) != 0;
      r = (r << 1) | (((i >= 64 ? __hi >> (i - 64) : __lo >> i)) & 1u);
      __q <<= 1;
      if (__carry || r >= c) {
        r -= c;
        __q |= 1;
      }
    }
    return {__q, r};
  }
}

// m ticks of num/den s as `__whole` units of `__unit` s (low 64 bits), `__secs` (< unit) whole seconds
// and `rem` (< den) ticks of 1/den s.
template <class _Tp>
struct __chrono_parts {
  unsigned long long __whole, __secs;
  _Tp rem;
};
constexpr __chrono_parts<unsigned long long> __chrono_split(unsigned long long m, unsigned long long num,
                                                        unsigned long long den, unsigned long long __unit) noexcept {
  // m * num / den = (m / den) * num + (m % den) * num / den
  const __chrono_qr a = ::__ycxx::__detail::__chrono_muldiv(m / den, num, __unit);
  const __chrono_qr b = ::__ycxx::__detail::__chrono_muldiv(m % den, num, den); // b.q < num
  const unsigned long long t = a.r + b.__q;
  return {a.__q + t / __unit, t % __unit, b.r};
}

// Floating-point counts: the same steps in long double (exact for integral values).
constexpr long double __chrono_floor(long double __x) noexcept { // x >= 0
  return __x < 9.2e18L ? static_cast<long double>(static_cast<unsigned long long>(__x)) : __x;
}
constexpr unsigned long long __chrono_to_ull(long double __x) noexcept { // x >= 0
  return __x < 18446744073709551615.0L ? static_cast<unsigned long long>(__x) : ~0ull;
}
constexpr __chrono_parts<long double> __chrono_split(long double m, unsigned long long num, unsigned long long den,
                                                 unsigned long long __unit) noexcept {
  const long double n = static_cast<long double>(num), d = static_cast<long double>(den),
                    __u = static_cast<long double>(__unit);
  auto __mod = [](long double __x, long double y, long double __q) { // x - q * y, in [0, y)
    const long double r = __x - __q * y;
    return r < 0 ? 0 : r >= y ? y - 1 : r;
  };
  const long double __q = ::__ycxx::__detail::__chrono_floor(m / d), r = __mod(m, d, __q);
  const long double a = __q * n, __aq = ::__ycxx::__detail::__chrono_floor(a / __u), __ar = __mod(a, __u, __aq);
  const long double __rn = r * n, __bq = ::__ycxx::__detail::__chrono_floor(__rn / d), __br = __mod(__rn, d, __bq);
  const long double t = __ar + __bq, __tq = ::__ycxx::__detail::__chrono_floor(t / __u);
  return {::__ycxx::__detail::__chrono_to_ull(__aq + __tq), ::__ycxx::__detail::__chrono_to_ull(__mod(t, __u, __tq)), __br};
}

// rem ticks of 1/den s as `width` fractional digits (truncated).
constexpr unsigned long long __chrono_frac(unsigned long long rem, unsigned long long den, unsigned width) noexcept {
  unsigned long long p = 1;
  for (unsigned i = 0; i != width; ++i)
    p *= 10;
  return ::__ycxx::__detail::__chrono_muldiv(rem, p, den).__q;
}
constexpr unsigned long long __chrono_frac(long double rem, unsigned long long den, unsigned width) noexcept {
  long double p = 1;
  for (unsigned i = 0; i != width; ++i)
    p *= 10;
  return ::__ycxx::__detail::__chrono_to_ull(::__ycxx::__detail::__chrono_floor(rem * p / static_cast<long double>(den)));
}

// The counts split without hh_mm_ss: the standard integer types up to 64 bits and the floating-
// point types. Others (wider integers, class types emulating arithmetic) go through hh_mm_ss.
template <class _Rep>
inline constexpr bool __chrono_plain_rep =
    (std::is_integral_v<_Rep> && sizeof(_Rep) <= sizeof(long long)) || std::is_floating_point_v<_Rep>;

// |c| as unsigned long long or long double (a NaN as 0).
template <class _Rep>
constexpr auto __chrono_magnitude(_Rep c) noexcept {
  if constexpr (std::is_floating_point_v<_Rep>) {
    const long double __x = static_cast<long double>(c);
    return __x < 0 ? -__x : __x > 0 ? __x : 0.0L;
  } else if constexpr (std::is_signed_v<_Rep>) {
    const long long __v = static_cast<long long>(c);
    return __v < 0 ? 0ull - static_cast<unsigned long long>(__v) : static_cast<unsigned long long>(__v);
  } else {
    return static_cast<unsigned long long>(c);
  }
}

// The time of day (or duration) d: hours (not reduced modulo 24), minutes, seconds and the
// fractional digits of hh_mm_ss<Dur>::fractional_width ([time.hms.members]).
template <class __charT, class _Rep, class _Period>
constexpr void __chrono_set_time(__chrono_fields<__charT>& __f, const std::chrono::duration<_Rep, _Period>& d) {
  using _Dur = std::chrono::duration<_Rep, _Period>;
  if constexpr (__chrono_plain_rep<_Rep>) {
    using _Pp = typename _Period::type;
    constexpr unsigned width = ::__ycxx::__detail::__hms_fractional_width(_Pp::den);
    __f.__negative = d.count() < _Rep(0);
    const auto __parts = ::__ycxx::__detail::__chrono_split(::__ycxx::__detail::__chrono_magnitude(d.count()),
                                                    static_cast<unsigned long long>(_Pp::num),
                                                    static_cast<unsigned long long>(_Pp::den), 3600);
    __f.hours = __parts.__whole;
    __f.minutes = static_cast<unsigned>(__parts.__secs / 60);
    __f.seconds = static_cast<unsigned>(__parts.__secs % 60);
    __f.width = width;
    __f.subseconds = ::__ycxx::__detail::__chrono_frac(__parts.rem, static_cast<unsigned long long>(_Pp::den), width);
  } else {
    const std::chrono::hh_mm_ss<_Dur> h(d);
    using __prec = typename std::chrono::hh_mm_ss<_Dur>::precision;
    __f.__negative = h.is_negative();
    __f.hours = static_cast<unsigned long long>(h.hours().count());
    __f.minutes = static_cast<unsigned>(h.minutes().count());
    __f.seconds = static_cast<unsigned>(h.seconds().count());
    __f.width = std::chrono::hh_mm_ss<_Dur>::fractional_width;
    if constexpr (std::chrono::treat_as_floating_point_v<typename __prec::rep>)
      __f.subseconds = static_cast<unsigned long long>(
          std::chrono::duration_cast<std::chrono::duration<long long, typename __prec::period>>(h.subseconds()).count());
    else
      __f.subseconds = static_cast<unsigned long long>(h.subseconds().count());
  }
}

// A time point's date and time of day, from its time since the epoch plus `days` days.
template <class __charT, class _Rep, class _Period>
constexpr void __chrono_set_point(__chrono_fields<__charT>& __f, const std::chrono::duration<_Rep, _Period>& __since_epoch,
                                long long days = 0) {
  using _Duration = std::chrono::duration<_Rep, _Period>;
  if constexpr (__chrono_plain_rep<_Rep>) {
    // The magnitude in whole days, seconds and ticks; a negative one is floored.
    using _Pp = typename _Period::type;
    constexpr unsigned long long den = static_cast<unsigned long long>(_Pp::den);
    constexpr unsigned width = ::__ycxx::__detail::__hms_fractional_width(_Pp::den);
    auto __parts = ::__ycxx::__detail::__chrono_split(::__ycxx::__detail::__chrono_magnitude(__since_epoch.count()),
                                              static_cast<unsigned long long>(_Pp::num), den, 86400);
    long long day = static_cast<long long>(__parts.__whole);
    if (__since_epoch.count() < _Rep(0)) {
      day = -day;
      if (__parts.__secs != 0 || __parts.rem != 0) {
        --day;
        if (__parts.rem != 0) {
          __parts.rem = static_cast<decltype(__parts.rem)>(den) - __parts.rem;
          __parts.__secs = 86399 - __parts.__secs;
        } else {
          __parts.__secs = 86400 - __parts.__secs;
        }
      }
    }
    ::__ycxx::__detail::__chrono_set_days(__f, static_cast<long long>(::__ycxx::__detail::__wrap_add(day, days)));
    __f.hours = __parts.__secs / 3600;
    __f.minutes = static_cast<unsigned>(__parts.__secs / 60 % 60);
    __f.seconds = static_cast<unsigned>(__parts.__secs % 60);
    __f.width = width;
    __f.subseconds = ::__ycxx::__detail::__chrono_frac(__parts.rem, den, width);
  } else {
    // Day and time of day by a floored division of the count, which cannot overflow even for
    // time_point::min() (floor<days> would compare in the finer type).
    using __cd = std::common_type_t<_Duration, std::chrono::days>;
    const __cd c(__since_epoch);
    const auto __ticks = __cd(std::chrono::days(1)).count();
    auto __q = c.count() / __ticks;
    auto r = c.count() % __ticks;
    if (r < 0) {
      --__q;
      r += __ticks;
    }
    const long long day = static_cast<long long>(::__ycxx::__detail::__wrap_add(static_cast<long long>(__q), days));
    ::__ycxx::__detail::__chrono_set_days(__f, day);
    ::__ycxx::__detail::__chrono_set_time(__f, __cd(r));
  }
}

// [time.duration.io]/1: the units suffix.
template <class __charT>
struct __chrono_suffix_text {
  __charT __text[48];
  std::size_t __len;
};
template <class _Period, class __charT>
constexpr __chrono_suffix_text<__charT> __chrono_suffix() noexcept {
  __chrono_suffix_text<__charT> r{};
  auto put = [&r](const char* s) {
    for (; *s != 0; ++s)
      r.__text[r.__len++] = static_cast<__charT>(*s);
  };
  using _Pp = typename _Period::type;
  using std::ratio;
  if constexpr (__is_same(_Pp, std::atto))
    put("as");
  else if constexpr (__is_same(_Pp, std::femto))
    put("fs");
  else if constexpr (__is_same(_Pp, std::pico))
    put("ps");
  else if constexpr (__is_same(_Pp, std::nano))
    put("ns");
  else if constexpr (__is_same(_Pp, std::micro)) {
    // "µs" where literals are Unicode, else "us".
    if constexpr (__uni::encoding<__charT> == 8) {
      r.__text[r.__len++] = static_cast<__charT>(0xC2);
      r.__text[r.__len++] = static_cast<__charT>(0xB5);
      put("s");
    } else if constexpr (__uni::encoding<__charT> != 0) {
      r.__text[r.__len++] = static_cast<__charT>(0xB5);
      put("s");
    } else {
      put("us");
    }
  } else if constexpr (__is_same(_Pp, std::milli))
    put("ms");
  else if constexpr (__is_same(_Pp, std::centi))
    put("cs");
  else if constexpr (__is_same(_Pp, std::deci))
    put("ds");
  else if constexpr (__is_same(_Pp, ratio<1>))
    put("s");
  else if constexpr (__is_same(_Pp, std::deca))
    put("das");
  else if constexpr (__is_same(_Pp, std::hecto))
    put("hs");
  else if constexpr (__is_same(_Pp, std::kilo))
    put("ks");
  else if constexpr (__is_same(_Pp, std::mega))
    put("Ms");
  else if constexpr (__is_same(_Pp, std::giga))
    put("Gs");
  else if constexpr (__is_same(_Pp, std::tera))
    put("Ts");
  else if constexpr (__is_same(_Pp, std::peta))
    put("Ps");
  else if constexpr (__is_same(_Pp, std::exa))
    put("Es");
  else if constexpr (__is_same(_Pp, ratio<60>))
    put("min");
  else if constexpr (__is_same(_Pp, ratio<3600>))
    put("h");
  else if constexpr (__is_same(_Pp, ratio<86400>))
    put("d");
  else {
    char digits[48];
    char* const end = digits + sizeof digits;
    char* p = end;
    *--p = 0;
    *--p = 's';
    *--p = ']';
    if constexpr (_Pp::den != 1) {
      p = ::__ycxx::__detail::__charconv_write_unsigned(p, static_cast<unsigned long long>(_Pp::den), 10);
      *--p = '/';
    }
    p = ::__ycxx::__detail::__charconv_write_unsigned(p, static_cast<unsigned long long>(_Pp::num), 10);
    *--p = '[';
    put(p);
  }
  return r;
}

// %Q: the count of a duration's magnitude, as "{}" formats it.
template <class __charT, class _Rep, class _Period>
constexpr void __chrono_set_duration(__chrono_fields<__charT>& __f, const std::chrono::duration<_Rep, _Period>& d) {
  __f.__is_duration = true;
  ::__ycxx::__detail::__chrono_set_time(__f, d);
  const __chrono_suffix_text<__charT> s = ::__ycxx::__detail::__chrono_suffix<_Period, __charT>();
  for (std::size_t i = 0; i != s.__len; ++i)
    __f.suffix[i] = s.__text[i];
  __f.__suffix_len = s.__len;
  const _Rep c = d.count();
  if constexpr (std::chrono::treat_as_floating_point_v<_Rep>) {
    const auto r = std::to_chars(__f.count, __f.count + sizeof __f.count, c < _Rep(0) ? -c : c);
    __f.__count_len = static_cast<std::size_t>(r.ptr - __f.count);
  } else if constexpr (std::is_signed_v<_Rep>) {
    const unsigned long long m = c < _Rep(0) ? 0ull - static_cast<unsigned long long>(static_cast<long long>(c))
                                            : static_cast<unsigned long long>(c);
    char* p = ::__ycxx::__detail::__charconv_write_unsigned(__f.count + sizeof __f.count, m, 10);
    __f.__count_len = static_cast<std::size_t>(__f.count + sizeof __f.count - p);
    for (std::size_t i = 0; i != __f.__count_len; ++i)
      __f.count[i] = p[i];
  } else {
    char* p = ::__ycxx::__detail::__charconv_write_unsigned(__f.count + sizeof __f.count, static_cast<unsigned long long>(c), 10);
    __f.__count_len = static_cast<std::size_t>(__f.count + sizeof __f.count - p);
    for (std::size_t i = 0; i != __f.__count_len; ++i)
      __f.count[i] = p[i];
  }
}

// ---- the locale-dependent conversions (src/hosted/chrono.cpp) ---------------------------------

struct __chrono_c_tm {
  int __sec, min, __hour, __mday, __mon, year, __wday, __yday;
  // the time zone, for a locale's representation that shows it (%c in some locales): the
  // abbreviation (null: none, so no zone is written) and the offset when known
  const char* __zone;
  bool __has_offset;
  long long offset; // seconds east of UTC
};
// Appends what loc's time_put<charT> writes for %<mod><spec> of t.
void __chrono_put_localized(std::string& out, const std::locale& __loc, const __chrono_c_tm& t, char __spec, char __mod);
void __chrono_put_localized(std::wstring& out, const std::locale& __loc, const __chrono_c_tm& t, char __spec, char __mod);
// Whether loc's time_put<charT> is the classic locale's facet (that of "C", "POSIX" and
// "C.UTF-8"; a named locale has its time_put_byname), whose conventions are the "C" locale's.
bool __chrono_classic_time_put(const std::locale& __loc, char);
bool __chrono_classic_time_put(const std::locale& __loc, wchar_t);
// Whether loc's num_put<charT> is the classic locale's facet (every named locale shares it,
// DECISIONS §7): its stage 2 is then computed from numpunct here, without a stream.
bool __chrono_classic_num_put(const std::locale& __loc, char);
bool __chrono_classic_num_put(const std::locale& __loc, wchar_t);
// Appends what `s << __v` writes to a stream s imbued with loc, with its default flags and the
// given precision: loc's num_put<charT> (a program's own) called for the count of a duration.
void __chrono_put_count(std::string& out, const std::locale& __loc, long __v, int precision);
void __chrono_put_count(std::string& out, const std::locale& __loc, unsigned long __v, int precision);
void __chrono_put_count(std::string& out, const std::locale& __loc, long long __v, int precision);
void __chrono_put_count(std::string& out, const std::locale& __loc, unsigned long long __v, int precision);
void __chrono_put_count(std::string& out, const std::locale& __loc, double __v, int precision);
void __chrono_put_count(std::string& out, const std::locale& __loc, long double __v, int precision);
void __chrono_put_count(std::wstring& out, const std::locale& __loc, long __v, int precision);
void __chrono_put_count(std::wstring& out, const std::locale& __loc, unsigned long __v, int precision);
void __chrono_put_count(std::wstring& out, const std::locale& __loc, long long __v, int precision);
void __chrono_put_count(std::wstring& out, const std::locale& __loc, unsigned long long __v, int precision);
void __chrono_put_count(std::wstring& out, const std::locale& __loc, double __v, int precision);
void __chrono_put_count(std::wstring& out, const std::locale& __loc, long double __v, int precision);
// The argument `s << c` passes to num_put::put for a count c ([ostream.inserters.arithmetic]/1:
// short and int as long, unsigned short and unsigned int as unsigned long, float as double).
// Character and extended integer reps keep their number (as the formatter writes them).
template <class _Rep>
constexpr auto __chrono_put_arg(_Rep c) noexcept {
  if constexpr (std::is_same_v<_Rep, long double>)
    return c;
  else if constexpr (std::is_floating_point_v<_Rep>)
    return static_cast<double>(c);
  else if constexpr (std::is_same_v<_Rep, long long> || std::is_same_v<_Rep, unsigned long long> ||
                     std::is_same_v<_Rep, unsigned long>)
    return c;
  else if constexpr (std::is_unsigned_v<_Rep> && sizeof(_Rep) <= sizeof(unsigned long))
    return static_cast<unsigned long>(c);
  else if constexpr (sizeof(_Rep) <= sizeof(long))
    return static_cast<long>(c);
  else if constexpr (std::is_unsigned_v<_Rep>)
    return static_cast<unsigned long long>(c);
  else
    return static_cast<long long>(c);
}

inline constexpr const char* __chrono_weekday_names[7] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                                                        "Thursday", "Friday", "Saturday"};
inline constexpr const char* __chrono_month_names[12] = {"January", "February", "March",     "April",
                                                       "May",     "June",     "July",      "August",
                                                       "September", "October", "November", "December"};

// ---- writing ----------------------------------------------------------------------------------

// [time.format]/2: the formatting locale is the "C" locale without the L option. With it, the
// locale's numpunct gives %S its decimal point, its num_put writes the counts of the default
// duration format (stage 2 computed here from numpunct for the classic num_put), and its
// time_put writes the locale-dependent conversions (names, %c %x
// %X %r %p and the E/O forms), unless it is the classic facet: the "C" locale's conventions are
// then built in (as without L). That keeps "{:L...}" with the "C" locale identical to "{:...}"
// also where a C tm cannot carry the value (hours of a duration beyond 23, years before 1 or
// after 9999, whose %Y the chrono formatter pads to four digits while strftime does not).
template <class __charT>
struct __chrono_out {
  __fmt_dynbuf<__charT>& b;
  const std::locale* __loc; // the formatting locale when the L option is given, else null ("C")
  bool __own_time = false;  // loc has a time_put of its own (not the classic one)

  __chrono_out(__fmt_dynbuf<__charT>& __buf, const std::locale* __l)
      : b(__buf), __loc(__l), __own_time(__l != nullptr && !::__ycxx::__detail::__chrono_classic_time_put(*__l, __charT())) {}

  void __ch(char c) { b.push_back(static_cast<__charT>(c)); }
  void __ascii(const char* s, std::size_t n) {
    for (std::size_t i = 0; i != n; ++i)
      b.push_back(static_cast<__charT>(s[i]));
  }
  void __ascii(const char* s) {
    for (; *s != 0; ++s)
      b.push_back(static_cast<__charT>(*s));
  }
  // v in decimal with at least `digits` digits, padded with `__pad`.
  void num(unsigned long long __v, int digits, char __pad = '0') {
    char d[24];
    char* const end = d + sizeof d;
    char* p = ::__ycxx::__detail::__charconv_write_unsigned(end, __v, 10);
    for (int n = static_cast<int>(end - p); n < digits; ++n)
      __ch(__pad);
    __ascii(p, static_cast<std::size_t>(end - p));
  }
  void __snum(long long __v, int digits) {
    if (__v < 0)
      __ch('-');
    num(__v < 0 ? 0ull - static_cast<unsigned long long>(__v) : static_cast<unsigned long long>(__v), digits);
  }
  __charT decimal_point() const {
    if (__loc == nullptr)
      return __charT('.');
    return std::use_facet<std::numpunct<__charT>>(*__loc).decimal_point();
  }
  // [p, e): a count as num_put writes it with loc ([facet.num.put.virtuals] stage 2: digit
  // groups of the integer part separated by thousands_sep, the locale's decimal point), or as
  // is without L.
  void count(const char* p, const char* e) {
    if (__loc == nullptr)
      return __ascii(p, static_cast<std::size_t>(e - p));
    const std::numpunct<__charT>& __np = std::use_facet<std::numpunct<__charT>>(*__loc);
    const std::string __g = __np.grouping();
    const __charT __sep = __np.thousands_sep();
    if (p != e && *p == '-')
      __ch(*p++);
    const char* d = p;
    while (d != e && *d >= '0' && *d <= '9')
      ++d;
    // sep_before[i]: a separator precedes integer digit i
    bool __sep_before[128] = {};
    const std::size_t n = static_cast<std::size_t>(d - p);
    for (std::size_t t = 0, done = 0; n <= 128;) {
      const std::size_t __sz = ::__ycxx::__detail::__fmt_group_size(__g, t++);
      if (__sz == 0 || done + __sz >= n)
        break;
      done += __sz;
      __sep_before[n - done] = true;
    }
    for (std::size_t i = 0; i != n; ++i) {
      if (__sep_before[i])
        b.push_back(__sep);
      __ch(p[i]);
    }
    for (; d != e; ++d)
      *d == '.' ? b.push_back(__np.decimal_point()) : __ch(*d);
  }
  void __localized(const __chrono_fields<__charT>& __f, char __spec, char __mod) {
    // The hour count of a duration where a tm can hold it (as %H writes it without L), reduced
    // modulo 24 for the 12-hour forms and AM/PM.
    const bool __twelve = __spec == 'I' || __spec == 'p' || __spec == 'r';
    const unsigned long long h =
        __twelve || __f.hours > static_cast<unsigned long long>(__INT_MAX__) ? __f.hours % 24 : __f.hours;
    __chrono_c_tm t{static_cast<int>(__f.seconds),
                        static_cast<int>(__f.minutes),
                        static_cast<int>(h),
                        static_cast<int>(__f.day),
                        static_cast<int>(__f.month) - 1,
                        __f.year - 1900,
                        static_cast<int>(__f.weekday),
                        __f.__yday,
                        nullptr,
                        __f.__has_offset,
                        __f.offset};
    const std::string __zone(__f.__has_abbrev ? __f.abbrev : std::string_view());
    if (__f.__has_abbrev)
      t.__zone = __zone.c_str();
    std::basic_string<__charT> s;
    ::__ycxx::__detail::__chrono_put_localized(s, *__loc, t, __spec, __mod);
    b.append(s.data(), s.size());
  }
};

[[noreturn]] inline void __chrono_missing(const char* what) { ::__ycxx::__detail::__throw_format_error(what); }

constexpr unsigned __chrono_iso_weeks(int y) noexcept {
  const unsigned __wd = ::__ycxx::__detail::__weekday_from_days(::__ycxx::__detail::__days_from_civil(y, 1, 1));
  return __wd == 4 || (__wd == 3 && ::__ycxx::__detail::__is_leap_year(y)) ? 53u : 52u;
}
// The ISO 8601 week-based year and week of f's date.
template <class __charT>
constexpr void __chrono_iso_week(const __chrono_fields<__charT>& __f, int& year, unsigned& __week) noexcept {
  const int __wd_mon = static_cast<int>((__f.weekday + 6) % 7);
  int __w = (__f.__yday - __wd_mon + 10) / 7;
  year = __f.year;
  if (__w < 1) {
    --year;
    __w = static_cast<int>(::__ycxx::__detail::__chrono_iso_weeks(year));
  } else if (__w > static_cast<int>(::__ycxx::__detail::__chrono_iso_weeks(year))) {
    ++year;
    __w = 1;
  }
  __week = static_cast<unsigned>(__w);
}

template <class __charT, class _PC>
void __chrono_write_specs(__chrono_out<__charT>& __o, const _PC* p, std::type_identity_t<const _PC*> e,
                        const __chrono_fields<__charT>& __f, bool sign);

// f without its subseconds: the locale's representations %c, %r and %X show whole seconds.
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_whole_seconds(__chrono_fields<__charT> __f) noexcept {
  __f.width = 0;
  return __f;
}

// One conversion specifier.
template <class __charT>
void __chrono_write_one(__chrono_out<__charT>& __o, const __chrono_fields<__charT>& __f, char __spec, char __mod) {
  const bool _Lp = __o.__own_time;
  auto __weekday_name = [&](bool __full) {
    if (!__f.__weekday_ok || __f.weekday > 6)
      ::__ycxx::__detail::__chrono_missing("std::format: the value does not contain a valid weekday");
    if (_Lp)
      return __o.__localized(__f, __full ? 'A' : 'a', 0);
    const char* n = __chrono_weekday_names[__f.weekday];
    __full ? __o.__ascii(n) : __o.__ascii(n, 3);
  };
  auto __month_name = [&](bool __full) {
    if (__f.month < 1 || __f.month > 12)
      ::__ycxx::__detail::__chrono_missing("std::format: the value does not contain a valid month");
    if (_Lp)
      return __o.__localized(__f, __spec, 0); // %b, %h or %B, as written
    const char* n = __chrono_month_names[__f.month - 1];
    __full ? __o.__ascii(n) : __o.__ascii(n, 3);
  };
  // The day of the year and the week numbers need a date ([time.format]/3: "If the formatted
  // object does not contain the information the conversion specifier refers to, an exception
  // of type format_error is thrown"); a !ok() year_month_day and the like name no day.
  auto __valid_date = [&] {
    if (!__f.__date_ok)
      ::__ycxx::__detail::__chrono_missing("std::format: the value does not contain a valid date");
  };
  auto __secs = [&](bool __fraction) {
    if (_Lp && __mod == 'O')
      __o.__localized(__f, 'S', 'O');
    else
      __o.num(__f.seconds, 2);
    if (__fraction && __f.width != 0) {
      __o.b.push_back(__o.decimal_point());
      __o.num(__f.subseconds, static_cast<int>(__f.width));
    }
  };
  // The E and O forms of the locale; in the "C" locale they are the plain forms.
  if (_Lp && __mod != 0 && __spec != 'z' && __spec != 'S') {
    if (__spec == 'U' || __spec == 'V' || __spec == 'W')
      __valid_date();
    else if ((__spec == 'u' || __spec == 'w') && (!__f.__weekday_ok || __f.weekday > 6))
      ::__ycxx::__detail::__chrono_missing("std::format: the value does not contain a valid weekday");
    return __o.__localized(__f, __spec, __mod);
  }
  switch (__spec) {
  case 'a': return __weekday_name(false);
  case 'A': return __weekday_name(true);
  case 'b':
  case 'h': return __month_name(false);
  case 'B': return __month_name(true);
  case 'c':
    if (_Lp)
      return __o.__localized(__f, 'c', __mod);
    return ::__ycxx::__detail::__chrono_write_specs(__o, "%a %b %e %H:%M:%S %Y", nullptr,
                                              ::__ycxx::__detail::__chrono_whole_seconds(__f), false);
  case 'C': {
    const long long c = ::__ycxx::__detail::__chrono_floor_div(__f.year, 100);
    return __o.__snum(c, 2);
  }
  case 'd': return __o.num(__f.day, 2);
  case 'e': return __o.num(__f.day, 2, ' ');
  case 'D': return ::__ycxx::__detail::__chrono_write_specs(__o, "%m/%d/%y", nullptr, __f, false);
  case 'F': return ::__ycxx::__detail::__chrono_write_specs(__o, "%Y-%m-%d", nullptr, __f, false);
  case 'g':
  case 'G': {
    __valid_date();
    int y;
    unsigned __w;
    ::__ycxx::__detail::__chrono_iso_week(__f, y, __w);
    if (__spec == 'g')
      return __o.num(static_cast<unsigned>(y < 0 ? -y : y) % 100, 2);
    return __o.__snum(y, 4);
  }
  case 'H': return __o.num(__f.hours, 2);
  case 'I': {
    const unsigned long long h = __f.hours % 12;
    return __o.num(h == 0 ? 12 : h, 2);
  }
  case 'j':
    if (__f.__is_duration)
      return __o.num(__f.hours / 24, 1);
    __valid_date();
    return __o.num(static_cast<unsigned long long>(__f.__yday + 1), 3);
  case 'm': return __o.num(__f.month, 2);
  case 'M': return __o.num(__f.minutes, 2);
  case 'n': return __o.__ch('\n');
  case 'p':
    if (_Lp)
      return __o.__localized(__f, 'p', 0);
    return __o.__ascii(__f.hours % 24 < 12 ? "AM" : "PM");
  case 'q': return __o.b.append(__f.suffix, __f.__suffix_len);
  case 'Q': return __o.__ascii(__f.count, __f.__count_len);
  case 'r':
    if (_Lp)
      return __o.__localized(__f, 'r', 0);
    return ::__ycxx::__detail::__chrono_write_specs(__o, "%I:%M:%S %p", nullptr, ::__ycxx::__detail::__chrono_whole_seconds(__f),
                                              false);
  case 'R':
    return ::__ycxx::__detail::__chrono_write_specs(__o, "%H:%M", nullptr, __f, false);
  case 'S': return __secs(true);
  case 't': return __o.__ch('\t');
  case 'T':
    ::__ycxx::__detail::__chrono_write_specs(__o, "%H:%M", nullptr, __f, false);
    __o.__ch(':');
    return __secs(true);
  case 'u':
    if (!__f.__weekday_ok || __f.weekday > 6)
      ::__ycxx::__detail::__chrono_missing("std::format: the value does not contain a valid weekday");
    return __o.num(__f.weekday == 0 ? 7 : __f.weekday, 1);
  case 'w':
    if (!__f.__weekday_ok || __f.weekday > 6)
      ::__ycxx::__detail::__chrono_missing("std::format: the value does not contain a valid weekday");
    return __o.num(__f.weekday, 1);
  case 'U':
    __valid_date();
    return __o.num(static_cast<unsigned>((__f.__yday + 7 - static_cast<int>(__f.weekday)) / 7), 2);
  case 'W':
    __valid_date();
    return __o.num(static_cast<unsigned>((__f.__yday + 7 - static_cast<int>((__f.weekday + 6) % 7)) / 7), 2);
  case 'V': {
    __valid_date();
    int y;
    unsigned __w;
    ::__ycxx::__detail::__chrono_iso_week(__f, y, __w);
    return __o.num(__w, 2);
  }
  case 'x':
    if (_Lp)
      return __o.__localized(__f, 'x', __mod);
    return ::__ycxx::__detail::__chrono_write_specs(__o, "%m/%d/%y", nullptr, __f, false);
  case 'X':
    if (_Lp)
      return __o.__localized(__f, 'X', __mod);
    return ::__ycxx::__detail::__chrono_write_specs(__o, "%H:%M:%S", nullptr, ::__ycxx::__detail::__chrono_whole_seconds(__f), false);
  case 'y': return __o.num(static_cast<unsigned>(__f.year < 0 ? -__f.year : __f.year) % 100, 2);
  case 'Y': return __o.__snum(__f.year, 4);
  case 'z': {
    if (!__f.__has_offset)
      ::__ycxx::__detail::__chrono_missing("std::format: the value has no time zone offset (%z)");
    const long long m = (__f.offset < 0 ? -__f.offset : __f.offset) / 60;
    __o.__ch(__f.offset < 0 ? '-' : '+');
    __o.num(static_cast<unsigned long long>(m / 60), 2);
    if (__mod != 0)
      __o.__ch(':');
    return __o.num(static_cast<unsigned long long>(m % 60), 2);
  }
  case 'Z':
    if (!__f.__has_abbrev)
      ::__ycxx::__detail::__chrono_missing("std::format: the value has no time zone abbreviation (%Z)");
    for (char c : __f.abbrev)
      __o.b.push_back(static_cast<__charT>(static_cast<unsigned char>(c)));
    return;
  case '%': return __o.__ch('%');
  default: ::__ycxx::__detail::__throw_format_error("std::format: invalid chrono conversion specifier");
  }
}

// The chrono-specs [p, e) (or the NTBS p when e is null); with `sign`, a negative duration gets
// its '-' before the first conversion specifier ([time.format]/4).
template <class __charT, class _PC>
void __chrono_write_specs(__chrono_out<__charT>& __o, const _PC* p, std::type_identity_t<const _PC*> e,
                        const __chrono_fields<__charT>& __f, bool sign) {
  for (; e == nullptr ? *p != _PC(0) : p != e; ++p) {
    if (*p != _PC('%')) {
      __o.b.push_back(static_cast<__charT>(*p));
      continue;
    }
    ++p;
    char __mod = 0;
    if (*p == _PC('E') || *p == _PC('O'))
      __mod = static_cast<char>(*p++);
    if (sign) {
      __o.__ch('-');
      sign = false;
    }
    ::__ycxx::__detail::__chrono_write_one(__o, __f, static_cast<char>(*p), __mod);
  }
}

// ---- chrono-format-spec ------------------------------------------------------------------------

template <class __charT>
struct __chrono_spec {
  __fmt_spec<__charT> std;
  const __charT* first = nullptr; // the chrono-specs
  const __charT* last = nullptr;
};

// What a specifier needs (0: nothing), or ~0u for an invalid one.
constexpr unsigned __chrono_needs(char __spec, char __mod) noexcept {
  constexpr const char* __e_ok = "cCxXyYz";
  constexpr const char* __o_ok = "deHImMSuUVwWyz";
  if (__mod != 0) {
    bool ok = false;
    for (const char* __q = __mod == 'E' ? __e_ok : __o_ok; *__q != 0; ++__q)
      ok = ok || *__q == __spec;
    if (!ok)
      return ~0u;
  }
  switch (__spec) {
  case 'a': case 'A': case 'u': case 'w': return __ci_weekday;
  case 'b': case 'B': case 'h': case 'm': return __ci_month;
  case 'C': case 'y': case 'Y': return __ci_year;
  case 'd': case 'e': return __ci_day;
  case 'D': case 'F': case 'x': return __ci_year | __ci_month | __ci_day;
  case 'g': case 'G': case 'U': case 'V': case 'W': return __ci_date;
  case 'j': return __ci_date; // or ci_duration, see below
  case 'c': return __ci_date | __ci_time;
  case 'H': case 'I': case 'M': case 'S': case 'p': case 'r': case 'R': case 'T': case 'X': return __ci_time;
  case 'q': case 'Q': return __ci_duration;
  case 'z': return __ci_offset;
  case 'Z': return __ci_zone;
  case 'n': case 't': case '%': return 0;
  default: return ~0u;
  }
}

// Parses a chrono-format-spec for a type with `info`; `__precision_ok` for a floating-point duration.
template <class __charT>
constexpr const __charT* __chrono_parse_spec(std::basic_format_parse_context<__charT>& __pc, __chrono_spec<__charT>& s,
                                         unsigned info, bool __precision_ok) {
  const __charT* p = __pc.begin();
  const __charT* const e = __pc.end();
  p = ::__ycxx::__detail::__fmt_parse_fill_align(p, e, s.std);
  p = ::__ycxx::__detail::__fmt_parse_width(__pc, p, e, s.std);
  if (p != e && *p == __charT('.')) {
    ++p;
    if (p != e && ::__ycxx::__detail::__fmt_is_digit(*p)) {
      p = ::__ycxx::__detail::__fmt_parse_number(p, e, s.std.precision);
      s.std.__prec_kind = __fmt_dyn::value;
    } else if (p != e && *p == __charT('{')) {
      p = ::__ycxx::__detail::__fmt_parse_dynamic(__pc, p + 1, e, s.std.precision);
      s.std.__prec_kind = __fmt_dyn::arg;
    } else {
      ::__ycxx::__detail::__throw_format_error("std::format: missing precision after '.'");
    }
    if (!__precision_ok)
      ::__ycxx::__detail::__throw_format_error(
          "std::format: a precision is valid only for durations with a floating-point representation");
  }
  if (p != e && *p == __charT('L'))
    s.std.__localized = true, ++p;
  s.first = p;
  if (p != e && *p != __charT('}') && *p != __charT('%'))
    ::__ycxx::__detail::__throw_format_error("std::format: chrono-specs must begin with a conversion specifier");
  while (p != e && *p != __charT('}')) {
    if (*p == __charT('{'))
      ::__ycxx::__detail::__throw_format_error("std::format: '{' in chrono-specs");
    if (*p != __charT('%')) {
      ++p;
      continue;
    }
    ++p;
    char __mod = 0;
    if (p != e && (*p == __charT('E') || *p == __charT('O')))
      __mod = static_cast<char>(*p++);
    if (p == e || static_cast<unsigned>(*p) > 127u)
      ::__ycxx::__detail::__throw_format_error("std::format: incomplete chrono conversion specifier");
    const char __spec = static_cast<char>(*p++);
    unsigned __need = ::__ycxx::__detail::__chrono_needs(__spec, __mod);
    if (__need == ~0u)
      ::__ycxx::__detail::__throw_format_error("std::format: invalid chrono conversion specifier");
    if (__spec == 'j' && (info & __ci_duration) != 0)
      __need = __ci_duration;
    if ((__need & info) != __need)
      ::__ycxx::__detail::__throw_format_error(
          "std::format: the formatted type does not contain the information a conversion specifier refers to");
  }
  s.last = p;
  return p;
}

// Pads the text in b as the std-format-spec asks.
template <class __charT, class _Context>
typename _Context::iterator __chrono_emit(const __chrono_spec<__charT>& s, _Context& __ctx, const __fmt_dynbuf<__charT>& b) {
  const std::size_t width = ::__ycxx::__detail::__fmt_width(s.std, __ctx);
  if (width == 0)
    return ::__ycxx::__detail::__fmt_put<__charT>(__ctx.out(), b.data(), b.size());
  return ::__ycxx::__detail::__fmt_write_padded<__charT>(__ctx.out(), s.std, __fmt_align::left, width,
                                                 __uni::width(b.data(), b.size()), b.data(), b.size());
}

// ---- the representations without chrono-specs (the stream inserters') ---------------------

template <class __charT, class _Rep, class _Period>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::duration<_Rep, _Period>& d, long long precision = -1) {
  // As `ostringstream s; s << d.count() << __units-suffix` with the formatting locale
  // ([time.format]/7, [time.duration.io]/1): a num_put of the locale's own is called.
  const _Rep c = d.count();
  const __chrono_suffix_text<__charT> s = ::__ycxx::__detail::__chrono_suffix<_Period, __charT>();
  if constexpr (std::is_arithmetic_v<_Rep>) {
    if (__o.__loc != nullptr && !::__ycxx::__detail::__chrono_classic_num_put(*__o.__loc, __charT())) {
      std::basic_string<__charT> __text;
      ::__ycxx::__detail::__chrono_put_count(__text, *__o.__loc, ::__ycxx::__detail::__chrono_put_arg(c),
                                             precision < 0 ? 6 : static_cast<int>(precision < 100 ? precision : 100));
      __o.b.append(__text.data(), __text.size());
      __o.b.append(s.__text, s.__len);
      return;
    }
  }
  char __buf[128];
  std::to_chars_result r;
  if constexpr (std::chrono::treat_as_floating_point_v<_Rep>)
    r = std::to_chars(__buf, __buf + sizeof __buf, c, std::chars_format::general,
                      precision < 0 ? 6 : static_cast<int>(precision < 100 ? precision : 100));
  else if constexpr (std::is_signed_v<_Rep>)
    r = std::to_chars(__buf, __buf + sizeof __buf, static_cast<long long>(c));
  else
    r = std::to_chars(__buf, __buf + sizeof __buf, static_cast<unsigned long long>(c));
  __o.count(__buf, r.ptr);
  __o.b.append(s.__text, s.__len);
}

template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::day& d) {
  __chrono_fields<__charT> __f;
  __f.day = static_cast<unsigned>(d);
  __o.num(__f.day, 2);
  if (!d.ok())
    __o.__ascii(" is not a valid day");
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::month& m) {
  if (m.ok()) {
    __chrono_fields<__charT> __f;
    __f.month = static_cast<unsigned>(m);
    return ::__ycxx::__detail::__chrono_write_one(__o, __f, 'b', 0);
  }
  __o.num(static_cast<unsigned>(m), 1);
  __o.__ascii(" is not a valid month");
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::year& y) {
  __o.__snum(static_cast<int>(y), 4);
  if (!y.ok())
    __o.__ascii(" is not a valid year");
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::weekday& __wd) {
  if (__wd.ok()) {
    __chrono_fields<__charT> __f;
    __f.weekday = __wd.c_encoding();
    __f.__weekday_ok = true;
    return ::__ycxx::__detail::__chrono_write_one(__o, __f, 'a', 0);
  }
  __o.num(__wd.c_encoding(), 1);
  __o.__ascii(" is not a valid weekday");
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::weekday_indexed& __wdi) {
  ::__ycxx::__detail::__chrono_default(__o, __wdi.weekday());
  __o.__ch('[');
  __o.num(__wdi.index(), 1);
  if (__wdi.index() < 1 || __wdi.index() > 5)
    __o.__ascii(" is not a valid index");
  __o.__ch(']');
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::weekday_last& __wdl) {
  ::__ycxx::__detail::__chrono_default(__o, __wdl.weekday());
  __o.__ascii("[last]");
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::month_day& __md) {
  ::__ycxx::__detail::__chrono_default(__o, __md.month());
  __o.__ch('/');
  ::__ycxx::__detail::__chrono_default(__o, __md.day());
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::month_day_last& __mdl) {
  ::__ycxx::__detail::__chrono_default(__o, __mdl.month());
  __o.__ascii("/last");
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::month_weekday& __mwd) {
  ::__ycxx::__detail::__chrono_default(__o, __mwd.month());
  __o.__ch('/');
  ::__ycxx::__detail::__chrono_default(__o, __mwd.weekday_indexed());
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::month_weekday_last& __mwdl) {
  ::__ycxx::__detail::__chrono_default(__o, __mwdl.month());
  __o.__ch('/');
  ::__ycxx::__detail::__chrono_default(__o, __mwdl.weekday_last());
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::year_month& __ym) {
  ::__ycxx::__detail::__chrono_default(__o, __ym.year());
  __o.__ch('/');
  ::__ycxx::__detail::__chrono_default(__o, __ym.month());
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::year_month_day& ymd) {
  __chrono_fields<__charT> __f;
  ::__ycxx::__detail::__chrono_set_ymd(__f, ymd);
  ::__ycxx::__detail::__chrono_write_specs(__o, "%Y-%m-%d", nullptr, __f, false);
  if (!ymd.ok())
    __o.__ascii(" is not a valid date");
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::year_month_day_last& __ymdl) {
  ::__ycxx::__detail::__chrono_default(__o, __ymdl.year());
  __o.__ch('/');
  ::__ycxx::__detail::__chrono_default(__o, __ymdl.month_day_last());
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::year_month_weekday& __ymwd) {
  ::__ycxx::__detail::__chrono_default(__o, __ymwd.year());
  __o.__ch('/');
  ::__ycxx::__detail::__chrono_default(__o, __ymwd.month());
  __o.__ch('/');
  ::__ycxx::__detail::__chrono_default(__o, __ymwd.weekday_indexed());
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::year_month_weekday_last& __ymwdl) {
  ::__ycxx::__detail::__chrono_default(__o, __ymwdl.year());
  __o.__ch('/');
  ::__ycxx::__detail::__chrono_default(__o, __ymwdl.month());
  __o.__ch('/');
  ::__ycxx::__detail::__chrono_default(__o, __ymwdl.weekday_last());
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::sys_info& i) {
  __chrono_fields<__charT> __f;
  __o.__ch('[');
  ::__ycxx::__detail::__chrono_set_point(__f, i.begin.time_since_epoch());
  ::__ycxx::__detail::__chrono_write_specs(__o, "%F %T", nullptr, __f, false);
  __o.__ascii(", ");
  __f = __chrono_fields<__charT>();
  ::__ycxx::__detail::__chrono_set_point(__f, i.end.time_since_epoch());
  ::__ycxx::__detail::__chrono_write_specs(__o, "%F %T", nullptr, __f, false);
  __o.__ascii(") ");
  __f = __chrono_fields<__charT>();
  ::__ycxx::__detail::__chrono_set_time(__f, i.offset);
  ::__ycxx::__detail::__chrono_write_specs(__o, "%T", nullptr, __f, __f.__negative);
  __o.__ch(' ');
  __o.__snum(i.save.count(), 1);
  __o.__ascii("min \"");
  for (char c : i.abbrev)
    __o.b.push_back(static_cast<__charT>(static_cast<unsigned char>(c)));
  __o.__ch('"');
}
template <class __charT>
void __chrono_default(__chrono_out<__charT>& __o, const std::chrono::local_info& i) {
  switch (i.result) {
  case std::chrono::local_info::unique: __o.__ascii("unique: "); break;
  case std::chrono::local_info::nonexistent: __o.__ascii("nonexistent: "); break;
  case std::chrono::local_info::ambiguous: __o.__ascii("ambiguous: "); break;
  default:
    __o.__ascii("unspecified result (");
    __o.__snum(i.result, 1);
    __o.__ascii("): ");
  }
  __o.__ch('{');
  ::__ycxx::__detail::__chrono_default(__o, i.first);
  __o.__ascii(", ");
  ::__ycxx::__detail::__chrono_default(__o, i.second);
  __o.__ch('}');
}

// The text of a value with chrono-specs (or with the default ones when there are none).
template <class __charT, class _PC>
void __chrono_render(__chrono_out<__charT>& __o, const __chrono_fields<__charT>& __f, const _PC* first, const _PC* last,
                   const char* __dflt) {
  if (first != last)
    ::__ycxx::__detail::__chrono_write_specs(__o, first, last, __f, __f.__negative);
  else
    ::__ycxx::__detail::__chrono_write_specs(__o, __dflt, nullptr, __f, __f.__negative);
}

// The fields of each formattable type and the information it has.
template <class __charT, class _Rep, class _Period>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::duration<_Rep, _Period>& d) {
  __chrono_fields<__charT> __f;
  ::__ycxx::__detail::__chrono_set_duration(__f, d);
  return __f;
}
template <class __charT, class _Duration>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::sys_time<_Duration>& t) {
  __chrono_fields<__charT> __f;
  ::__ycxx::__detail::__chrono_set_point(__f, t.time_since_epoch());
  __f.__has_abbrev = __f.__has_offset = true;
  __f.abbrev = "UTC";
  return __f;
}
template <class __charT, class _Duration>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::local_time<_Duration>& t) {
  __chrono_fields<__charT> __f;
  ::__ycxx::__detail::__chrono_set_point(__f, t.time_since_epoch());
  return __f;
}
template <class __charT, class _Duration>
__chrono_fields<__charT> __chrono_fields_of(const std::chrono::utc_time<_Duration>& t) {
  using __cd = std::common_type_t<_Duration, std::chrono::seconds>;
  const std::chrono::leap_second_info __lsi = std::chrono::get_leap_second_info(t);
  __chrono_fields<__charT> __f;
  ::__ycxx::__detail::__chrono_set_point(__f, __cd(t.time_since_epoch()) - __lsi.elapsed);
  if (__lsi.is_leap_second)
    __f.seconds = 60; // [time.format]/11: during a leap second the seconds are 60
  __f.__has_abbrev = __f.__has_offset = true;
  __f.abbrev = "UTC";
  return __f;
}
template <class __charT, class _Duration>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::tai_time<_Duration>& t) {
  // [time.format]/12: the sys_time 1958-01-01 is the TAI epoch (4383 days before 1970).
  __chrono_fields<__charT> __f;
  ::__ycxx::__detail::__chrono_set_point(__f, t.time_since_epoch(), -4383);
  __f.__has_abbrev = __f.__has_offset = true;
  __f.abbrev = "TAI";
  return __f;
}
template <class __charT, class _Duration>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::gps_time<_Duration>& t) {
  // [time.format]/13: the GPS epoch is 1980-01-06, 3657 days after 1970-01-01.
  __chrono_fields<__charT> __f;
  ::__ycxx::__detail::__chrono_set_point(__f, t.time_since_epoch(), 3657);
  __f.__has_abbrev = __f.__has_offset = true;
  __f.abbrev = "GPS";
  return __f;
}
template <class __charT, class _Duration>
__chrono_fields<__charT> __chrono_fields_of(const std::chrono::file_time<_Duration>& t) {
  return ::__ycxx::__detail::__chrono_fields_of<__charT>(std::chrono::clock_cast<std::chrono::system_clock>(t));
}
template <class __charT, class _Duration>
constexpr __chrono_fields<__charT> __chrono_fields_of(const __ycxx::__adl_free::__local_time_format_t<_Duration>& t) {
  __chrono_fields<__charT> __f;
  ::__ycxx::__detail::__chrono_set_point(__f, t.__time_.time_since_epoch());
  if (t.__abbrev_ != nullptr) {
    __f.__has_abbrev = true;
    __f.abbrev = *t.__abbrev_;
  }
  if (t.__offset_sec_ != nullptr) {
    __f.__has_offset = true;
    __f.offset = t.__offset_sec_->count();
  }
  return __f;
}
template <class __charT, class _Duration>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::hh_mm_ss<_Duration>& h) {
  __chrono_fields<__charT> __f;
  ::__ycxx::__detail::__chrono_set_time(__f, h.to_duration());
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::day& d) {
  __chrono_fields<__charT> __f;
  __f.day = static_cast<unsigned>(d);
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::month& m) {
  __chrono_fields<__charT> __f;
  __f.month = static_cast<unsigned>(m);
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::year& y) {
  __chrono_fields<__charT> __f;
  __f.year = static_cast<int>(y);
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::weekday& __wd) {
  __chrono_fields<__charT> __f;
  __f.weekday = __wd.c_encoding();
  __f.__weekday_ok = __wd.ok();
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::weekday_indexed& __wdi) {
  return ::__ycxx::__detail::__chrono_fields_of<__charT>(__wdi.weekday());
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::weekday_last& __wdl) {
  return ::__ycxx::__detail::__chrono_fields_of<__charT>(__wdl.weekday());
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::month_day& __md) {
  __chrono_fields<__charT> __f;
  __f.month = static_cast<unsigned>(__md.month());
  __f.day = static_cast<unsigned>(__md.day());
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::month_day_last& __mdl) {
  __chrono_fields<__charT> __f;
  __f.month = static_cast<unsigned>(__mdl.month());
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::month_weekday& __mwd) {
  __chrono_fields<__charT> __f = ::__ycxx::__detail::__chrono_fields_of<__charT>(__mwd.weekday_indexed().weekday());
  __f.month = static_cast<unsigned>(__mwd.month());
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::month_weekday_last& __mwdl) {
  __chrono_fields<__charT> __f = ::__ycxx::__detail::__chrono_fields_of<__charT>(__mwdl.weekday_last().weekday());
  __f.month = static_cast<unsigned>(__mwdl.month());
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::year_month& __ym) {
  __chrono_fields<__charT> __f;
  __f.year = static_cast<int>(__ym.year());
  __f.month = static_cast<unsigned>(__ym.month());
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::year_month_day& ymd) {
  __chrono_fields<__charT> __f;
  ::__ycxx::__detail::__chrono_set_ymd(__f, ymd);
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::year_month_day_last& __ymdl) {
  return ::__ycxx::__detail::__chrono_fields_of<__charT>(std::chrono::year_month_day(__ymdl));
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::year_month_weekday& __ymwd) {
  __chrono_fields<__charT> __f;
  if (__ymwd.ok()) {
    ::__ycxx::__detail::__chrono_set_days(__f, std::chrono::sys_days(__ymwd).time_since_epoch().count());
  } else {
    __f.year = static_cast<int>(__ymwd.year());
    __f.month = static_cast<unsigned>(__ymwd.month());
    __f.weekday = __ymwd.weekday().c_encoding();
    __f.__weekday_ok = __ymwd.weekday().ok();
  }
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::year_month_weekday_last& __ymwdl) {
  __chrono_fields<__charT> __f;
  if (__ymwdl.ok()) {
    ::__ycxx::__detail::__chrono_set_days(__f, std::chrono::sys_days(__ymwdl).time_since_epoch().count());
  } else {
    __f.year = static_cast<int>(__ymwdl.year());
    __f.month = static_cast<unsigned>(__ymwdl.month());
    __f.weekday = __ymwdl.weekday().c_encoding();
    __f.__weekday_ok = __ymwdl.weekday().ok();
  }
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::sys_info& i) {
  __chrono_fields<__charT> __f;
  __f.__has_abbrev = __f.__has_offset = true;
  __f.abbrev = i.abbrev;
  __f.offset = i.offset.count();
  return __f;
}
template <class __charT>
constexpr __chrono_fields<__charT> __chrono_fields_of(const std::chrono::local_info&) {
  return __chrono_fields<__charT>();
}

// The information of each type, and its default chrono-specs (null: a chrono_default overload).
template <class _Tp>
struct __chrono_traits {
  static constexpr unsigned info = __ci_full_date | __ci_time;
  static constexpr const char* __dflt = "%F %T";
};
template <class _Rep, class _Period>
struct __chrono_traits<std::chrono::duration<_Rep, _Period>> {
  static constexpr unsigned info = __ci_time | __ci_duration;
  static constexpr const char* __dflt = nullptr;
};
template <class _Duration>
struct __chrono_traits<std::chrono::sys_time<_Duration>> {
  static constexpr unsigned info = __ci_full_date | __ci_time | __ci_zone | __ci_offset;
  // os << sys_days is os << year_month_day{dp}. (Duration{1} < days{1}, compared as ratios: in
  // the common type a day of a fine period such as femto overflows.)
  static constexpr const char* __dflt = std::chrono::treat_as_floating_point_v<typename _Duration::rep> ||
                                              std::ratio_less_v<typename _Duration::period, std::ratio<86400>>
                                          ? "%F %T"
                                          : "%F";
};
template <class _Duration>
struct __chrono_traits<std::chrono::local_time<_Duration>> {
  static constexpr unsigned info = __ci_full_date | __ci_time;
  static constexpr const char* __dflt = __chrono_traits<std::chrono::sys_time<_Duration>>::__dflt;
};
template <class _Duration>
struct __chrono_traits<std::chrono::utc_time<_Duration>> : __chrono_traits<std::chrono::sys_time<_Duration>> {
  static constexpr const char* __dflt = "%F %T";
};
template <class _Duration>
struct __chrono_traits<std::chrono::tai_time<_Duration>> : __chrono_traits<std::chrono::utc_time<_Duration>> {};
template <class _Duration>
struct __chrono_traits<std::chrono::gps_time<_Duration>> : __chrono_traits<std::chrono::utc_time<_Duration>> {};
template <class _Duration>
struct __chrono_traits<std::chrono::file_time<_Duration>> : __chrono_traits<std::chrono::utc_time<_Duration>> {};
template <class _Duration>
struct __chrono_traits<__ycxx::__adl_free::__local_time_format_t<_Duration>> {
  static constexpr unsigned info = __ci_full_date | __ci_time | __ci_zone | __ci_offset;
  static constexpr const char* __dflt = "%F %T %Z";
};
template <class _Duration>
struct __chrono_traits<std::chrono::hh_mm_ss<_Duration>> {
  static constexpr unsigned info = __ci_time;
  static constexpr const char* __dflt = "%T";
};
template <>
struct __chrono_traits<std::chrono::day> {
  static constexpr unsigned info = __ci_day;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::month> {
  static constexpr unsigned info = __ci_month;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::year> {
  static constexpr unsigned info = __ci_year;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::weekday> {
  static constexpr unsigned info = __ci_weekday;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::weekday_indexed> : __chrono_traits<std::chrono::weekday> {};
template <>
struct __chrono_traits<std::chrono::weekday_last> : __chrono_traits<std::chrono::weekday> {};
template <>
struct __chrono_traits<std::chrono::month_day> {
  static constexpr unsigned info = __ci_month | __ci_day;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::month_day_last> {
  static constexpr unsigned info = __ci_month;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::month_weekday> {
  static constexpr unsigned info = __ci_month | __ci_weekday;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::month_weekday_last> : __chrono_traits<std::chrono::month_weekday> {};
template <>
struct __chrono_traits<std::chrono::year_month> {
  static constexpr unsigned info = __ci_year | __ci_month;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::year_month_day> {
  static constexpr unsigned info = __ci_full_date;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::year_month_day_last> {
  static constexpr unsigned info = __ci_full_date;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::year_month_weekday> : __chrono_traits<std::chrono::year_month_day_last> {};
template <>
struct __chrono_traits<std::chrono::year_month_weekday_last> : __chrono_traits<std::chrono::year_month_day_last> {};
template <>
struct __chrono_traits<std::chrono::sys_info> {
  static constexpr unsigned info = __ci_zone | __ci_offset;
  static constexpr const char* __dflt = nullptr;
};
template <>
struct __chrono_traits<std::chrono::local_info> {
  static constexpr unsigned info = 0;
  static constexpr const char* __dflt = nullptr;
};

template <class _Tp>
inline constexpr bool __chrono_float_duration = false;
template <class _Rep, class _Period>
inline constexpr bool __chrono_float_duration<std::chrono::duration<_Rep, _Period>> =
    std::chrono::treat_as_floating_point_v<_Rep>;

// Writes v into b with the chrono-specs [first, last) (none: the default representation).
template <class __charT, class _Tp, class _PC>
void __chrono_text(__fmt_dynbuf<__charT>& b, const _Tp& __v, const _PC* first, const _PC* last, const std::locale* __loc,
                 long long precision = -1) {
  __chrono_out<__charT> __o{b, __loc};
  if (first == last && __chrono_traits<_Tp>::__dflt == nullptr) {
    if constexpr (requires { ::__ycxx::__detail::__chrono_default(__o, __v); }) {
      if constexpr (requires { typename _Tp::period; })
        ::__ycxx::__detail::__chrono_default(__o, __v, precision);
      else
        ::__ycxx::__detail::__chrono_default(__o, __v);
    }
    return;
  }
  const __chrono_fields<__charT> __f = ::__ycxx::__detail::__chrono_fields_of<__charT>(__v);
  ::__ycxx::__detail::__chrono_render(__o, __f, first, last, __chrono_traits<_Tp>::__dflt);
}

// The common formatter: parse() checks the chrono-format-spec against T's information.
template <class _Tp, class __charT>
struct __chrono_formatter {
  __chrono_spec<__charT> __spec_;

  constexpr typename std::basic_format_parse_context<__charT>::iterator
  parse(std::basic_format_parse_context<__charT>& __pc) {
    return ::__ycxx::__detail::__chrono_parse_spec(__pc, __spec_, __chrono_traits<_Tp>::info, __chrono_float_duration<_Tp>);
  }
  template <class _FormatContext>
  typename _FormatContext::iterator format(const _Tp& __v, _FormatContext& __ctx) const {
    __fmt_dynbuf<__charT> b;
    const long long precision = ::__ycxx::__detail::__fmt_precision(__spec_.std, __ctx);
    if (__spec_.std.__localized) {
      const std::locale __loc = __ctx.locale();
      ::__ycxx::__detail::__chrono_text(b, __v, __spec_.first, __spec_.last, __builtin_addressof(__loc), precision);
    } else {
      ::__ycxx::__detail::__chrono_text(b, __v, __spec_.first, __spec_.last, nullptr, precision);
    }
    return ::__ycxx::__detail::__chrono_emit(__spec_, __ctx, b);
  }
};

// The stream inserters: the default representation with the stream's locale.
template <class __charT, class __traits, class _Tp>
std::basic_ostream<__charT, __traits>& __chrono_insert(std::basic_ostream<__charT, __traits>& __os, const _Tp& __v,
                                                 const char* __specs = nullptr) {
  __fmt_dynbuf<__charT> b;
  const std::locale __loc = __os.getloc();
  if (__specs == nullptr)
    ::__ycxx::__detail::__chrono_text(b, __v, __specs, __specs, __builtin_addressof(__loc));
  else
    ::__ycxx::__detail::__chrono_text(b, __v, __specs, __specs + std::char_traits<char>::length(__specs), __builtin_addressof(__loc));
  return __os << std::basic_string_view<__charT, __traits>(b.data(), b.size());
}

// Formats as the stream inserter would with the "C" locale, into a std::string (the exception
// messages of [time.zone.exception]).
template <class _Tp>
void __chrono_append(std::string& s, const _Tp& __v) {
  __fmt_dynbuf<char> b;
  ::__ycxx::__detail::__chrono_text(b, __v, static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), nullptr);
  s.append(b.data(), b.size());
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [time.format]: the formatters.
template <class _Rep, class _Period, __ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::duration<_Rep, _Period>, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::duration<_Rep, _Period>, __charT> {};
template <class _Duration, __ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::sys_time<_Duration>, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::sys_time<_Duration>, __charT> {};
template <class _Duration, __ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::utc_time<_Duration>, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::utc_time<_Duration>, __charT> {};
template <class _Duration, __ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::tai_time<_Duration>, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::tai_time<_Duration>, __charT> {};
template <class _Duration, __ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::gps_time<_Duration>, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::gps_time<_Duration>, __charT> {};
template <class _Duration, __ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::file_time<_Duration>, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::file_time<_Duration>, __charT> {};
template <class _Duration, __ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::local_time<_Duration>, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::local_time<_Duration>, __charT> {};
template <class _Duration, __ycxx::__detail::__fmt_char __charT>
struct formatter<__ycxx::__adl_free::__local_time_format_t<_Duration>, __charT>
    : __ycxx::__detail::__chrono_formatter<__ycxx::__adl_free::__local_time_format_t<_Duration>, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::day, __charT> : __ycxx::__detail::__chrono_formatter<chrono::day, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::month, __charT> : __ycxx::__detail::__chrono_formatter<chrono::month, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::year, __charT> : __ycxx::__detail::__chrono_formatter<chrono::year, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::weekday, __charT> : __ycxx::__detail::__chrono_formatter<chrono::weekday, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::weekday_indexed, __charT> : __ycxx::__detail::__chrono_formatter<chrono::weekday_indexed, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::weekday_last, __charT> : __ycxx::__detail::__chrono_formatter<chrono::weekday_last, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::month_day, __charT> : __ycxx::__detail::__chrono_formatter<chrono::month_day, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::month_day_last, __charT> : __ycxx::__detail::__chrono_formatter<chrono::month_day_last, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::month_weekday, __charT> : __ycxx::__detail::__chrono_formatter<chrono::month_weekday, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::month_weekday_last, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::month_weekday_last, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::year_month, __charT> : __ycxx::__detail::__chrono_formatter<chrono::year_month, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::year_month_day, __charT> : __ycxx::__detail::__chrono_formatter<chrono::year_month_day, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::year_month_day_last, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::year_month_day_last, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::year_month_weekday, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::year_month_weekday, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::year_month_weekday_last, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::year_month_weekday_last, __charT> {};
template <class _Rep, class _Period, __ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::hh_mm_ss<chrono::duration<_Rep, _Period>>, __charT>
    : __ycxx::__detail::__chrono_formatter<chrono::hh_mm_ss<chrono::duration<_Rep, _Period>>, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::sys_info, __charT> : __ycxx::__detail::__chrono_formatter<chrono::sys_info, __charT> {};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::local_info, __charT> : __ycxx::__detail::__chrono_formatter<chrono::local_info, __charT> {};

// [time.format]/19
template <class _Duration, class _TimeZonePtr, __ycxx::__detail::__fmt_char __charT>
struct formatter<chrono::zoned_time<_Duration, _TimeZonePtr>, __charT>
    : formatter<__ycxx::__adl_free::__local_time_format_t<common_type_t<_Duration, chrono::seconds>>, __charT> {
  template <class _FormatContext>
  typename _FormatContext::iterator format(const chrono::zoned_time<_Duration, _TimeZonePtr>& __tp,
                                          _FormatContext& __ctx) const {
    const chrono::sys_info info = __tp.get_info();
    return formatter<__ycxx::__adl_free::__local_time_format_t<common_type_t<_Duration, chrono::seconds>>, __charT>::format(
        {__tp.get_local_time(), __builtin_addressof(info.abbrev), __builtin_addressof(info.offset)}, __ctx);
  }
};

// [format.formatter.spec]/3 and [time.format]/8-9.
template <class _Rep, class _Period>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::duration<_Rep, _Period>> =
    enable_nonlocking_formatter_optimization<_Rep>;
template <class _Duration>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::zoned_time<_Duration, const chrono::time_zone*>> =
    true;
template <class _Clock, class _Duration>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::time_point<_Clock, _Duration>> = true;
template <class _Duration>
inline constexpr bool enable_nonlocking_formatter_optimization<__ycxx::__adl_free::__local_time_format_t<_Duration>> = true;
template <class _Duration>
inline constexpr bool enable_nonlocking_formatter_optimization<chrono::hh_mm_ss<_Duration>> = true;
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

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

// [time.duration.io]/1
template <class __charT, class __traits, class _Rep, class _Period>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const duration<_Rep, _Period>& d) {
  // s.flags(os.flags()); s.imbue(os.getloc()); s.precision(os.precision()); s << d.count() << suffix,
  // through a stream on a string buffer of its own (no <sstream> needed).
  struct __string_buf : basic_streambuf<__charT, __traits> {
    basic_string<__charT, __traits> __text;
    typename __traits::int_type overflow(typename __traits::int_type c) override {
      if (!__traits::eq_int_type(c, __traits::eof()))
        __text.push_back(__traits::to_char_type(c));
      return __traits::not_eof(c);
    }
    ptrdiff_t xsputn(const __charT* p, ptrdiff_t n) override { // streamsize
      __text.append(p, static_cast<size_t>(n));
      return n;
    }
  } __buf;
  basic_ostream<__charT, __traits> s(__builtin_addressof(__buf));
  s.flags(__os.flags());
  s.imbue(__os.getloc());
  s.precision(__os.precision());
  s << d.count();
  const __ycxx::__detail::__chrono_suffix_text<__charT> suffix = __ycxx::__detail::__chrono_suffix<_Period, __charT>();
  __buf.__text.append(suffix.__text, suffix.__len);
  return __os << __buf.__text;
}

// [time.clock.system.nonmembers]/1-5
template <class __charT, class __traits, class _Duration>
  requires(!treat_as_floating_point_v<typename _Duration::rep> && _Duration(1) < days(1))
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const sys_time<_Duration>& __tp) {
  return __ycxx::__detail::__chrono_insert(__os, __tp, "%F %T");
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const sys_days& __dp) {
  return __ycxx::__detail::__chrono_insert(__os, year_month_day(__dp));
}
template <class __charT, class __traits, class _Duration>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const utc_time<_Duration>& t) {
  return __ycxx::__detail::__chrono_insert(__os, t, "%F %T");
}
template <class __charT, class __traits, class _Duration>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const tai_time<_Duration>& t) {
  return __ycxx::__detail::__chrono_insert(__os, t, "%F %T");
}
template <class __charT, class __traits, class _Duration>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const gps_time<_Duration>& t) {
  return __ycxx::__detail::__chrono_insert(__os, t, "%F %T");
}
template <class __charT, class __traits, class _Duration>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const file_time<_Duration>& t) {
  return __ycxx::__detail::__chrono_insert(__os, t, "%F %T");
}
// [time.clock.local]/2-4
template <class __charT, class __traits, class _Duration>
  requires requires(basic_ostream<__charT, __traits>& __os, const sys_time<_Duration>& __st) { __os << __st; }
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const local_time<_Duration>& lt) {
  return __os << sys_time<_Duration>(lt.time_since_epoch());
}

// [time.cal]
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const day& d) {
  return __ycxx::__detail::__chrono_insert(__os, d);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const month& m) {
  return __ycxx::__detail::__chrono_insert(__os, m);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const year& y) {
  return __ycxx::__detail::__chrono_insert(__os, y);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const weekday& __wd) {
  return __ycxx::__detail::__chrono_insert(__os, __wd);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const weekday_indexed& __wdi) {
  return __ycxx::__detail::__chrono_insert(__os, __wdi);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const weekday_last& __wdl) {
  return __ycxx::__detail::__chrono_insert(__os, __wdl);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const month_day& __md) {
  return __ycxx::__detail::__chrono_insert(__os, __md);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const month_day_last& __mdl) {
  return __ycxx::__detail::__chrono_insert(__os, __mdl);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const month_weekday& __mwd) {
  return __ycxx::__detail::__chrono_insert(__os, __mwd);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const month_weekday_last& __mwdl) {
  return __ycxx::__detail::__chrono_insert(__os, __mwdl);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const year_month& __ym) {
  return __ycxx::__detail::__chrono_insert(__os, __ym);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const year_month_day& ymd) {
  return __ycxx::__detail::__chrono_insert(__os, ymd);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const year_month_day_last& __ymdl) {
  return __ycxx::__detail::__chrono_insert(__os, __ymdl);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const year_month_weekday& __ymwd) {
  return __ycxx::__detail::__chrono_insert(__os, __ymwd);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const year_month_weekday_last& __ymwdl) {
  return __ycxx::__detail::__chrono_insert(__os, __ymwdl);
}

// [time.hms.nonmembers]
template <class __charT, class __traits, class _Duration>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const hh_mm_ss<_Duration>& hms) {
  return __ycxx::__detail::__chrono_insert(__os, hms, "%T");
}

// [time.zone.info]: unspecified formats.
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const sys_info& r) {
  return __ycxx::__detail::__chrono_insert(__os, r);
}
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const local_info& r) {
  return __ycxx::__detail::__chrono_insert(__os, r);
}

// [time.zone.zonedtime.nonmembers]/2
template <class __charT, class __traits, class _Duration, class _TimeZonePtr>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const zoned_time<_Duration, _TimeZonePtr>& t) {
  const sys_info info = t.get_info();
  return __ycxx::__detail::__chrono_insert(
      __os, local_time_format(t.get_local_time(), __builtin_addressof(info.abbrev), __builtin_addressof(info.offset)),
      "%F %T %Z");
}

// [time.zone.exception]
template <class _Duration>
nonexistent_local_time::nonexistent_local_time(const local_time<_Duration>& __tp, const local_info& i)
    : runtime_error([&] {
        string s;
        __ycxx::__detail::__chrono_append(s, __tp);
        s += " is in a gap between\n";
        __ycxx::__detail::__chrono_append(s, local_seconds(i.first.end.time_since_epoch()) + i.first.offset);
        s += ' ';
        s += i.first.abbrev;
        s += " and\n";
        __ycxx::__detail::__chrono_append(s, local_seconds(i.second.begin.time_since_epoch()) + i.second.offset);
        s += ' ';
        s += i.second.abbrev;
        s += " which are both equivalent to\n";
        __ycxx::__detail::__chrono_append(s, i.first.end);
        s += " UTC";
        return s;
      }()) {}

template <class _Duration>
ambiguous_local_time::ambiguous_local_time(const local_time<_Duration>& __tp, const local_info& i)
    : runtime_error([&] {
        string s;
        __ycxx::__detail::__chrono_append(s, __tp);
        s += " is ambiguous.  It could be\n";
        __ycxx::__detail::__chrono_append(s, __tp);
        s += ' ';
        s += i.first.abbrev;
        s += " == ";
        __ycxx::__detail::__chrono_append(s, __tp - i.first.offset);
        s += " UTC or\n";
        __ycxx::__detail::__chrono_append(s, __tp);
        s += ' ';
        s += i.second.abbrev;
        s += " == ";
        __ycxx::__detail::__chrono_append(s, __tp - i.second.offset);
        s += " UTC";
        return s;
      }()) {}

}} // namespace std::chrono
