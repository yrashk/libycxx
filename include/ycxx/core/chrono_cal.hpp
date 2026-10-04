// libycxx core: the civil calendar of <chrono> ([time.cal]), hh_mm_ss ([time.hms]), the 12/24
// hour functions ([time.12]) and the calendar parts of [time.hash]. Everything is constexpr and
// needs nothing from the OS; the stream operators, formatters and parsers are hosted
// (ycxx/hosted/chrono_io.hpp).
//
// Days and civil dates are converted with the era-based algorithm: a year starting on March 1st
// makes February the last month, so the day of the year is a linear function of the month, and
// 400-year eras (146097 days) make the conversion valid for every int year.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/core/compare.hpp>

namespace ycxx::detail {

// Floored division and modulo.
constexpr long long chrono_floor_div(long long a, long long b) noexcept {
  const long long q = a / b;
  return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}
constexpr long long chrono_modulo(long long a, long long b) noexcept {
  const long long r = a % b;
  return r != 0 && ((r < 0) != (b < 0)) ? r + b : r;
}
// a + b and a - b modulo 2^64: for results that are truncated to a narrow field anyway (day,
// year), and for counts at the ends of the range, without signed overflow.
constexpr unsigned long long wrap_add(long long a, long long b) noexcept {
  return static_cast<unsigned long long>(a) + static_cast<unsigned long long>(b);
}
constexpr unsigned long long wrap_sub(long long a, long long b) noexcept {
  return static_cast<unsigned long long>(a) - static_cast<unsigned long long>(b);
}

struct civil_date {
  int y;
  unsigned m;
  unsigned d;
};

// Days since 1970-01-01 of the proleptic Gregorian date y-m-d (m in [1, 12], d any value).
constexpr int days_from_civil(int y, unsigned m, unsigned d) noexcept {
  const long long yy = static_cast<long long>(y) - (m <= 2 ? 1 : 0);
  const long long era = ::ycxx::detail::chrono_floor_div(yy, 400);
  const long long yoe = yy - era * 400;                                              // [0, 399]
  const long long doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + static_cast<long long>(d) - 1; // [0, 365]
  const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                        // [0, 146096]
  return static_cast<int>(era * 146097 + doe - 719468);
}

// The proleptic Gregorian date of the day z days after 1970-01-01.
constexpr civil_date civil_from_days(long long z) noexcept {
  // z + 719468 days from 0000-03-01, split into eras without overflow for any z.
  const long long shifted = ::ycxx::detail::chrono_modulo(z, 146097) + 719468;
  const long long era = ::ycxx::detail::chrono_floor_div(z, 146097) + ::ycxx::detail::chrono_floor_div(shifted, 146097);
  const long long doe = ::ycxx::detail::chrono_modulo(shifted, 146097);      // [0, 146096]
  const long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
  const long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);              // [0, 365]
  const long long mp = (5 * doy + 2) / 153;                                   // [0, 11], March first
  const unsigned d = static_cast<unsigned>(doy - (153 * mp + 2) / 5 + 1);
  const unsigned m = static_cast<unsigned>(mp < 10 ? mp + 3 : mp - 9);
  return {static_cast<int>(yoe + era * 400 + (m <= 2 ? 1 : 0)), m, d};
}

// 0 for Sunday.
constexpr unsigned weekday_from_days(long long z) noexcept {
  return static_cast<unsigned>(::ycxx::detail::chrono_modulo(::ycxx::detail::chrono_modulo(z, 7) + 4, 7));
}

constexpr bool is_leap_year(int y) noexcept { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }

// The number of days of month m (in [1, 12]) of year y.
constexpr unsigned last_day_of(int y, unsigned m) noexcept {
  constexpr unsigned char table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return m == 2 && ::ycxx::detail::is_leap_year(y) ? 29u : table[m - 1];
}

// hh_mm_ss::fractional_width ([time.hms.members]/1): the smallest w in [0, 18] such that 10^w is
// a multiple of the reduced period's denominator, or 6.
consteval unsigned hms_fractional_width(std::intmax_t den) {
  unsigned twos = 0, fives = 0;
  while (den % 2 == 0)
    den /= 2, ++twos;
  while (den % 5 == 0)
    den /= 5, ++fives;
  const unsigned w = twos > fives ? twos : fives;
  return den == 1 && w <= 18 ? w : 6;
}
consteval std::intmax_t pow10(unsigned n) {
  std::intmax_t r = 1;
  while (n-- != 0)
    r *= 10;
  return r;
}

} // namespace ycxx::detail

namespace std::chrono {

// [time.cal.last]
struct last_spec {
  explicit last_spec() = default;
};
inline constexpr last_spec last{};

// [time.cal.day]
class day {
  unsigned char d_;

public:
  day() = default;
  constexpr explicit day(unsigned d) noexcept : d_(static_cast<unsigned char>(d)) {}
  constexpr day& operator++() noexcept {
    ++d_;
    return *this;
  }
  constexpr day operator++(int) noexcept {
    day t = *this;
    ++d_;
    return t;
  }
  constexpr day& operator--() noexcept {
    --d_;
    return *this;
  }
  constexpr day operator--(int) noexcept {
    day t = *this;
    --d_;
    return t;
  }
  constexpr day& operator+=(const days& d) noexcept {
    *this = day(static_cast<unsigned>(::ycxx::detail::wrap_add(d_, d.count())));
    return *this;
  }
  constexpr day& operator-=(const days& d) noexcept {
    *this = day(static_cast<unsigned>(::ycxx::detail::wrap_sub(d_, d.count())));
    return *this;
  }
  constexpr explicit operator unsigned() const noexcept { return d_; }
  constexpr bool ok() const noexcept { return d_ >= 1 && d_ <= 31; }
};

constexpr bool operator==(const day& x, const day& y) noexcept {
  return static_cast<unsigned>(x) == static_cast<unsigned>(y);
}
constexpr strong_ordering operator<=>(const day& x, const day& y) noexcept {
  return static_cast<unsigned>(x) <=> static_cast<unsigned>(y);
}
constexpr day operator+(const day& x, const days& y) noexcept {
  return day(static_cast<unsigned>(::ycxx::detail::wrap_add(static_cast<unsigned>(x), y.count())));
}
constexpr day operator+(const days& x, const day& y) noexcept { return y + x; }
constexpr day operator-(const day& x, const days& y) noexcept {
  return day(static_cast<unsigned>(::ycxx::detail::wrap_sub(static_cast<unsigned>(x), y.count())));
}
constexpr days operator-(const day& x, const day& y) noexcept {
  return days(static_cast<int>(static_cast<unsigned>(x)) - static_cast<int>(static_cast<unsigned>(y)));
}

// [time.cal.month]
class month {
  unsigned char m_;

public:
  month() = default;
  constexpr explicit month(unsigned m) noexcept : m_(static_cast<unsigned char>(m)) {}
  constexpr month& operator++() noexcept {
    *this += months(1);
    return *this;
  }
  constexpr month operator++(int) noexcept {
    month t = *this;
    ++*this;
    return t;
  }
  constexpr month& operator--() noexcept {
    *this -= months(1);
    return *this;
  }
  constexpr month operator--(int) noexcept {
    month t = *this;
    --*this;
    return t;
  }
  constexpr month& operator+=(const months& m) noexcept;
  constexpr month& operator-=(const months& m) noexcept;
  constexpr explicit operator unsigned() const noexcept { return m_; }
  constexpr bool ok() const noexcept { return m_ >= 1 && m_ <= 12; }
};

constexpr bool operator==(const month& x, const month& y) noexcept {
  return static_cast<unsigned>(x) == static_cast<unsigned>(y);
}
constexpr strong_ordering operator<=>(const month& x, const month& y) noexcept {
  return static_cast<unsigned>(x) <=> static_cast<unsigned>(y);
}
constexpr month operator+(const month& x, const months& y) noexcept {
  return month(static_cast<unsigned>(
      ::ycxx::detail::chrono_modulo(static_cast<long long>(static_cast<unsigned>(x)) - 1 +
                                        ::ycxx::detail::chrono_modulo(y.count(), 12),
                                    12) +
      1));
}
constexpr month operator+(const months& x, const month& y) noexcept { return y + x; }
constexpr month operator-(const month& x, const months& y) noexcept {
  return month(static_cast<unsigned>(
      ::ycxx::detail::chrono_modulo(static_cast<long long>(static_cast<unsigned>(x)) - 1 -
                                        ::ycxx::detail::chrono_modulo(y.count(), 12),
                                    12) +
      1));
}
constexpr months operator-(const month& x, const month& y) noexcept {
  return months(static_cast<int>(::ycxx::detail::chrono_modulo(
      static_cast<long long>(static_cast<unsigned>(x)) - static_cast<long long>(static_cast<unsigned>(y)), 12)));
}
constexpr month& month::operator+=(const months& m) noexcept {
  *this = *this + m;
  return *this;
}
constexpr month& month::operator-=(const months& m) noexcept {
  *this = *this - m;
  return *this;
}

// [time.cal.year]
class year {
  short y_;

public:
  year() = default;
  constexpr explicit year(int y) noexcept : y_(static_cast<short>(y)) {}
  constexpr year& operator++() noexcept {
    ++y_;
    return *this;
  }
  constexpr year operator++(int) noexcept {
    year t = *this;
    ++y_;
    return t;
  }
  constexpr year& operator--() noexcept {
    --y_;
    return *this;
  }
  constexpr year operator--(int) noexcept {
    year t = *this;
    --y_;
    return t;
  }
  constexpr year& operator+=(const years& y) noexcept {
    *this = year(static_cast<int>(::ycxx::detail::wrap_add(y_, y.count())));
    return *this;
  }
  constexpr year& operator-=(const years& y) noexcept {
    *this = year(static_cast<int>(::ycxx::detail::wrap_sub(y_, y.count())));
    return *this;
  }
  constexpr year operator+() const noexcept { return *this; }
  constexpr year operator-() const noexcept { return year(-y_); }
  constexpr bool is_leap() const noexcept { return ::ycxx::detail::is_leap_year(y_); }
  constexpr explicit operator int() const noexcept { return y_; }
  constexpr bool ok() const noexcept { return y_ != -32768; }
  static constexpr year min() noexcept { return year(-32767); }
  static constexpr year max() noexcept { return year(32767); }
};

constexpr bool operator==(const year& x, const year& y) noexcept { return static_cast<int>(x) == static_cast<int>(y); }
constexpr strong_ordering operator<=>(const year& x, const year& y) noexcept {
  return static_cast<int>(x) <=> static_cast<int>(y);
}
constexpr year operator+(const year& x, const years& y) noexcept {
  return year(static_cast<int>(::ycxx::detail::wrap_add(static_cast<int>(x), y.count())));
}
constexpr year operator+(const years& x, const year& y) noexcept { return y + x; }
constexpr year operator-(const year& x, const years& y) noexcept {
  return year(static_cast<int>(::ycxx::detail::wrap_sub(static_cast<int>(x), y.count())));
}
constexpr years operator-(const year& x, const year& y) noexcept {
  return years(static_cast<int>(x) - static_cast<int>(y));
}

class weekday_indexed;
class weekday_last;

// [time.cal.wd]
class weekday {
  unsigned char wd_;

public:
  weekday() = default;
  constexpr explicit weekday(unsigned wd) noexcept : wd_(static_cast<unsigned char>(wd == 7 ? 0 : wd)) {}
  constexpr weekday(const sys_days& dp) noexcept
      : wd_(static_cast<unsigned char>(::ycxx::detail::weekday_from_days(dp.time_since_epoch().count()))) {}
  constexpr explicit weekday(const local_days& dp) noexcept
      : wd_(static_cast<unsigned char>(::ycxx::detail::weekday_from_days(dp.time_since_epoch().count()))) {}
  constexpr weekday& operator++() noexcept {
    *this += days(1);
    return *this;
  }
  constexpr weekday operator++(int) noexcept {
    weekday t = *this;
    ++*this;
    return t;
  }
  constexpr weekday& operator--() noexcept {
    *this -= days(1);
    return *this;
  }
  constexpr weekday operator--(int) noexcept {
    weekday t = *this;
    --*this;
    return t;
  }
  constexpr weekday& operator+=(const days& d) noexcept;
  constexpr weekday& operator-=(const days& d) noexcept;
  constexpr unsigned c_encoding() const noexcept { return wd_; }
  constexpr unsigned iso_encoding() const noexcept { return wd_ == 0 ? 7u : wd_; }
  constexpr bool ok() const noexcept { return wd_ <= 6; }
  constexpr weekday_indexed operator[](unsigned index) const noexcept;
  constexpr weekday_last operator[](last_spec) const noexcept;
};

constexpr bool operator==(const weekday& x, const weekday& y) noexcept { return x.c_encoding() == y.c_encoding(); }
constexpr weekday operator+(const weekday& x, const days& y) noexcept {
  return weekday(static_cast<unsigned>(::ycxx::detail::chrono_modulo(
      static_cast<long long>(x.c_encoding()) + ::ycxx::detail::chrono_modulo(y.count(), 7), 7)));
}
constexpr weekday operator+(const days& x, const weekday& y) noexcept { return y + x; }
constexpr weekday operator-(const weekday& x, const days& y) noexcept {
  return weekday(static_cast<unsigned>(::ycxx::detail::chrono_modulo(
      static_cast<long long>(x.c_encoding()) - ::ycxx::detail::chrono_modulo(y.count(), 7), 7)));
}
constexpr days operator-(const weekday& x, const weekday& y) noexcept {
  return days(static_cast<int>(::ycxx::detail::chrono_modulo(
      static_cast<long long>(x.c_encoding()) - static_cast<long long>(y.c_encoding()), 7)));
}
constexpr weekday& weekday::operator+=(const days& d) noexcept {
  *this = *this + d;
  return *this;
}
constexpr weekday& weekday::operator-=(const days& d) noexcept {
  *this = *this - d;
  return *this;
}

// [time.cal.wdidx]
class weekday_indexed {
  chrono::weekday wd_;
  unsigned char index_;

public:
  weekday_indexed() = default;
  constexpr weekday_indexed(const chrono::weekday& wd, unsigned index) noexcept
      : wd_(wd), index_(static_cast<unsigned char>(index)) {}
  constexpr chrono::weekday weekday() const noexcept { return wd_; }
  constexpr unsigned index() const noexcept { return index_; }
  constexpr bool ok() const noexcept { return wd_.ok() && index_ >= 1 && index_ <= 5; }
};
constexpr bool operator==(const weekday_indexed& x, const weekday_indexed& y) noexcept {
  return x.weekday() == y.weekday() && x.index() == y.index();
}

// [time.cal.wdlast]
class weekday_last {
  chrono::weekday wd_;

public:
  constexpr explicit weekday_last(const chrono::weekday& wd) noexcept : wd_(wd) {}
  constexpr chrono::weekday weekday() const noexcept { return wd_; }
  constexpr bool ok() const noexcept { return wd_.ok(); }
};
constexpr bool operator==(const weekday_last& x, const weekday_last& y) noexcept { return x.weekday() == y.weekday(); }

constexpr weekday_indexed weekday::operator[](unsigned index) const noexcept { return {*this, index}; }
constexpr weekday_last weekday::operator[](last_spec) const noexcept { return weekday_last(*this); }

// [time.cal.md]
class month_day {
  chrono::month m_;
  chrono::day d_;

public:
  month_day() = default;
  constexpr month_day(const chrono::month& m, const chrono::day& d) noexcept : m_(m), d_(d) {}
  constexpr chrono::month month() const noexcept { return m_; }
  constexpr chrono::day day() const noexcept { return d_; }
  // February has 29 days here.
  constexpr bool ok() const noexcept {
    return m_.ok() && static_cast<unsigned>(d_) >= 1 &&
           static_cast<unsigned>(d_) <= ::ycxx::detail::last_day_of(2000, static_cast<unsigned>(m_));
  }
};
constexpr bool operator==(const month_day& x, const month_day& y) noexcept {
  return x.month() == y.month() && x.day() == y.day();
}
constexpr strong_ordering operator<=>(const month_day& x, const month_day& y) noexcept {
  if (auto c = x.month() <=> y.month(); c != 0)
    return c;
  return x.day() <=> y.day();
}

// [time.cal.mdlast]
class month_day_last {
  chrono::month m_;

public:
  constexpr explicit month_day_last(const chrono::month& m) noexcept : m_(m) {}
  constexpr chrono::month month() const noexcept { return m_; }
  constexpr bool ok() const noexcept { return m_.ok(); }
};
constexpr bool operator==(const month_day_last& x, const month_day_last& y) noexcept { return x.month() == y.month(); }
constexpr strong_ordering operator<=>(const month_day_last& x, const month_day_last& y) noexcept {
  return x.month() <=> y.month();
}

// [time.cal.mwd]
class month_weekday {
  chrono::month m_;
  chrono::weekday_indexed wdi_;

public:
  constexpr month_weekday(const chrono::month& m, const chrono::weekday_indexed& wdi) noexcept : m_(m), wdi_(wdi) {}
  constexpr chrono::month month() const noexcept { return m_; }
  constexpr chrono::weekday_indexed weekday_indexed() const noexcept { return wdi_; }
  constexpr bool ok() const noexcept { return m_.ok() && wdi_.ok(); }
};
constexpr bool operator==(const month_weekday& x, const month_weekday& y) noexcept {
  return x.month() == y.month() && x.weekday_indexed() == y.weekday_indexed();
}

// [time.cal.mwdlast]
class month_weekday_last {
  chrono::month m_;
  chrono::weekday_last wdl_;

public:
  constexpr month_weekday_last(const chrono::month& m, const chrono::weekday_last& wdl) noexcept : m_(m), wdl_(wdl) {}
  constexpr chrono::month month() const noexcept { return m_; }
  constexpr chrono::weekday_last weekday_last() const noexcept { return wdl_; }
  constexpr bool ok() const noexcept { return m_.ok() && wdl_.ok(); }
};
constexpr bool operator==(const month_weekday_last& x, const month_weekday_last& y) noexcept {
  return x.month() == y.month() && x.weekday_last() == y.weekday_last();
}

// The months overloads of year_month and the types built on it are templates (template <class =
// void>): for an argument convertible to both months and years, its conversion to years must be
// the better one ([time.cal.ym.members]/3 and friends), and a non-template wins the tie.

// [time.cal.ym]
class year_month;
} // namespace std::chrono

namespace ycxx::detail {
// ym + n months, or ym - n months with `subtract`, without signed overflow for any n.
constexpr std::chrono::year_month add_months(const std::chrono::year_month& ym, long long n,
                                             bool subtract = false) noexcept;
} // namespace ycxx::detail

namespace std::chrono {
class year_month {
  chrono::year y_;
  chrono::month m_;

public:
  year_month() = default;
  constexpr year_month(const chrono::year& y, const chrono::month& m) noexcept : y_(y), m_(m) {}
  constexpr chrono::year year() const noexcept { return y_; }
  constexpr chrono::month month() const noexcept { return m_; }
  template <class = void>
  constexpr year_month& operator+=(const months& dm) noexcept;
  template <class = void>
  constexpr year_month& operator-=(const months& dm) noexcept;
  constexpr year_month& operator+=(const years& dy) noexcept {
    y_ += dy;
    return *this;
  }
  constexpr year_month& operator-=(const years& dy) noexcept {
    y_ -= dy;
    return *this;
  }
  constexpr bool ok() const noexcept { return y_.ok() && m_.ok(); }
};
constexpr bool operator==(const year_month& x, const year_month& y) noexcept {
  return x.year() == y.year() && x.month() == y.month();
}
constexpr strong_ordering operator<=>(const year_month& x, const year_month& y) noexcept {
  if (auto c = x.year() <=> y.year(); c != 0)
    return c;
  return x.month() <=> y.month();
}
} // namespace std::chrono

constexpr std::chrono::year_month ycxx::detail::add_months(const std::chrono::year_month& ym, long long n,
                                                          bool subtract) noexcept {
  // n = 12q + r with r in [0, 11]; the month moves by r, the year by q and the carry.
  const long long q = ::ycxx::detail::chrono_floor_div(n, 12);
  const long long r = ::ycxx::detail::chrono_modulo(n, 12);
  const long long m = static_cast<long long>(static_cast<unsigned>(ym.month())) - 1 + (subtract ? -r : r);
  const long long carry = ::ycxx::detail::chrono_floor_div(m, 12);
  const unsigned long long y = (subtract ? ::ycxx::detail::wrap_sub(static_cast<int>(ym.year()), q)
                                         : ::ycxx::detail::wrap_add(static_cast<int>(ym.year()), q)) +
                               static_cast<unsigned long long>(carry);
  return {std::chrono::year(static_cast<int>(y)),
          std::chrono::month(static_cast<unsigned>(::ycxx::detail::chrono_modulo(m, 12) + 1))};
}

namespace std::chrono {
template <class = void>
constexpr year_month operator+(const year_month& ym, const months& dm) noexcept {
  return ::ycxx::detail::add_months(ym, dm.count());
}
template <class = void>
constexpr year_month operator+(const months& dm, const year_month& ym) noexcept {
  return ym + dm;
}
template <class = void>
constexpr year_month operator-(const year_month& ym, const months& dm) noexcept {
  return ::ycxx::detail::add_months(ym, dm.count(), true);
}
constexpr months operator-(const year_month& x, const year_month& y) noexcept {
  return months(
      (static_cast<int>(x.year()) - static_cast<int>(y.year())) * 12 +
      (static_cast<int>(static_cast<unsigned>(x.month())) - static_cast<int>(static_cast<unsigned>(y.month()))));
}
constexpr year_month operator+(const year_month& ym, const years& dy) noexcept { return {ym.year() + dy, ym.month()}; }
constexpr year_month operator+(const years& dy, const year_month& ym) noexcept { return ym + dy; }
constexpr year_month operator-(const year_month& ym, const years& dy) noexcept { return {ym.year() - dy, ym.month()}; }
template <class V>
constexpr year_month& year_month::operator+=(const months& dm) noexcept {
  *this = *this + dm;
  return *this;
}
template <class V>
constexpr year_month& year_month::operator-=(const months& dm) noexcept {
  *this = *this - dm;
  return *this;
}

class year_month_day_last;

// [time.cal.ymd]
class year_month_day {
  chrono::year y_;
  chrono::month m_;
  chrono::day d_;

  static constexpr year_month_day from_days(long long z) noexcept {
    const ycxx::detail::civil_date c = ::ycxx::detail::civil_from_days(z);
    return {chrono::year(c.y), chrono::month(c.m), chrono::day(c.d)};
  }
  constexpr days to_days() const noexcept {
    // For an invalid day of a valid year and month: sys_days{y/m/1d} + (d - 1d).
    return days(::ycxx::detail::days_from_civil(static_cast<int>(y_), static_cast<unsigned>(m_), 1) +
                (static_cast<int>(static_cast<unsigned>(d_)) - 1));
  }

public:
  year_month_day() = default;
  constexpr year_month_day(const chrono::year& y, const chrono::month& m, const chrono::day& d) noexcept
      : y_(y), m_(m), d_(d) {}
  constexpr year_month_day(const year_month_day_last& ymdl) noexcept;
  constexpr year_month_day(const sys_days& dp) noexcept : year_month_day(from_days(dp.time_since_epoch().count())) {}
  constexpr explicit year_month_day(const local_days& dp) noexcept
      : year_month_day(from_days(dp.time_since_epoch().count())) {}
  template <class = void>
  constexpr year_month_day& operator+=(const months& m) noexcept;
  template <class = void>
  constexpr year_month_day& operator-=(const months& m) noexcept;
  constexpr year_month_day& operator+=(const years& y) noexcept {
    y_ += y;
    return *this;
  }
  constexpr year_month_day& operator-=(const years& y) noexcept {
    y_ -= y;
    return *this;
  }
  constexpr chrono::year year() const noexcept { return y_; }
  constexpr chrono::month month() const noexcept { return m_; }
  constexpr chrono::day day() const noexcept { return d_; }
  constexpr operator sys_days() const noexcept { return sys_days(to_days()); }
  constexpr explicit operator local_days() const noexcept { return local_days(to_days()); }
  constexpr bool ok() const noexcept {
    return y_.ok() && m_.ok() && static_cast<unsigned>(d_) >= 1 &&
           static_cast<unsigned>(d_) <= ::ycxx::detail::last_day_of(static_cast<int>(y_), static_cast<unsigned>(m_));
  }
};
constexpr bool operator==(const year_month_day& x, const year_month_day& y) noexcept {
  return x.year() == y.year() && x.month() == y.month() && x.day() == y.day();
}
constexpr strong_ordering operator<=>(const year_month_day& x, const year_month_day& y) noexcept {
  if (auto c = x.year() <=> y.year(); c != 0)
    return c;
  if (auto c = x.month() <=> y.month(); c != 0)
    return c;
  return x.day() <=> y.day();
}
template <class = void>
constexpr year_month_day operator+(const year_month_day& ymd, const months& dm) noexcept {
  const year_month ym = year_month(ymd.year(), ymd.month()) + dm;
  return {ym.year(), ym.month(), ymd.day()};
}
template <class = void>
constexpr year_month_day operator+(const months& dm, const year_month_day& ymd) noexcept {
  return ymd + dm;
}
template <class = void>
constexpr year_month_day operator-(const year_month_day& ymd, const months& dm) noexcept {
  const year_month ym = ::ycxx::detail::add_months({ymd.year(), ymd.month()}, dm.count(), true);
  return {ym.year(), ym.month(), ymd.day()};
}
constexpr year_month_day operator+(const year_month_day& ymd, const years& dy) noexcept {
  return {ymd.year() + dy, ymd.month(), ymd.day()};
}
constexpr year_month_day operator+(const years& dy, const year_month_day& ymd) noexcept { return ymd + dy; }
constexpr year_month_day operator-(const year_month_day& ymd, const years& dy) noexcept {
  return {ymd.year() - dy, ymd.month(), ymd.day()};
}
template <class V>
constexpr year_month_day& year_month_day::operator+=(const months& m) noexcept {
  *this = *this + m;
  return *this;
}
template <class V>
constexpr year_month_day& year_month_day::operator-=(const months& m) noexcept {
  *this = *this - m;
  return *this;
}

// [time.cal.ymdlast]
class year_month_day_last {
  chrono::year y_;
  chrono::month_day_last mdl_;

public:
  constexpr year_month_day_last(const chrono::year& y, const chrono::month_day_last& mdl) noexcept
      : y_(y), mdl_(mdl) {}
  template <class = void>
  constexpr year_month_day_last& operator+=(const months& m) noexcept;
  template <class = void>
  constexpr year_month_day_last& operator-=(const months& m) noexcept;
  constexpr year_month_day_last& operator+=(const years& y) noexcept {
    y_ += y;
    return *this;
  }
  constexpr year_month_day_last& operator-=(const years& y) noexcept {
    y_ -= y;
    return *this;
  }
  constexpr chrono::year year() const noexcept { return y_; }
  constexpr chrono::month month() const noexcept { return mdl_.month(); }
  constexpr chrono::month_day_last month_day_last() const noexcept { return mdl_; }
  // For a month that is not ok() (the value is unspecified there): 31.
  constexpr chrono::day day() const noexcept {
    const unsigned m = static_cast<unsigned>(mdl_.month());
    return chrono::day(m >= 1 && m <= 12 ? ::ycxx::detail::last_day_of(static_cast<int>(y_), m) : 31u);
  }
  constexpr operator sys_days() const noexcept { return sys_days(year_month_day(y_, month(), day())); }
  constexpr explicit operator local_days() const noexcept {
    return local_days(sys_days(*this).time_since_epoch());
  }
  constexpr bool ok() const noexcept { return y_.ok() && mdl_.ok(); }
};
constexpr bool operator==(const year_month_day_last& x, const year_month_day_last& y) noexcept {
  return x.year() == y.year() && x.month_day_last() == y.month_day_last();
}
constexpr strong_ordering operator<=>(const year_month_day_last& x, const year_month_day_last& y) noexcept {
  if (auto c = x.year() <=> y.year(); c != 0)
    return c;
  return x.month_day_last() <=> y.month_day_last();
}
template <class = void>
constexpr year_month_day_last operator+(const year_month_day_last& ymdl, const months& dm) noexcept {
  const year_month ym = year_month(ymdl.year(), ymdl.month()) + dm;
  return {ym.year(), month_day_last(ym.month())};
}
template <class = void>
constexpr year_month_day_last operator+(const months& dm, const year_month_day_last& ymdl) noexcept {
  return ymdl + dm;
}
template <class = void>
constexpr year_month_day_last operator-(const year_month_day_last& ymdl, const months& dm) noexcept {
  const year_month ym = ::ycxx::detail::add_months({ymdl.year(), ymdl.month()}, dm.count(), true);
  return {ym.year(), month_day_last(ym.month())};
}
constexpr year_month_day_last operator+(const year_month_day_last& ymdl, const years& dy) noexcept {
  return {ymdl.year() + dy, ymdl.month_day_last()};
}
constexpr year_month_day_last operator+(const years& dy, const year_month_day_last& ymdl) noexcept {
  return ymdl + dy;
}
constexpr year_month_day_last operator-(const year_month_day_last& ymdl, const years& dy) noexcept {
  return {ymdl.year() - dy, ymdl.month_day_last()};
}
template <class V>
constexpr year_month_day_last& year_month_day_last::operator+=(const months& m) noexcept {
  *this = *this + m;
  return *this;
}
template <class V>
constexpr year_month_day_last& year_month_day_last::operator-=(const months& m) noexcept {
  *this = *this - m;
  return *this;
}

constexpr year_month_day::year_month_day(const year_month_day_last& ymdl) noexcept
    : y_(ymdl.year()), m_(ymdl.month()), d_(ymdl.day()) {}

// [time.cal.ymwd]
class year_month_weekday {
  chrono::year y_;
  chrono::month m_;
  chrono::weekday_indexed wdi_;

  static constexpr year_month_weekday from_days(long long z) noexcept {
    const ycxx::detail::civil_date c = ::ycxx::detail::civil_from_days(z);
    return {chrono::year(c.y), chrono::month(c.m),
            chrono::weekday_indexed(chrono::weekday(::ycxx::detail::weekday_from_days(z)), (c.d - 1) / 7 + 1)};
  }
  constexpr days to_days() const noexcept {
    const int first = ::ycxx::detail::days_from_civil(static_cast<int>(y_), static_cast<unsigned>(m_), 1);
    const chrono::weekday wd1(::ycxx::detail::weekday_from_days(first));
    return days(first + (wdi_.weekday() - wd1).count() + (static_cast<int>(wdi_.index()) - 1) * 7);
  }

public:
  year_month_weekday() = default;
  constexpr year_month_weekday(const chrono::year& y, const chrono::month& m,
                               const chrono::weekday_indexed& wdi) noexcept
      : y_(y), m_(m), wdi_(wdi) {}
  constexpr year_month_weekday(const sys_days& dp) noexcept
      : year_month_weekday(from_days(dp.time_since_epoch().count())) {}
  constexpr explicit year_month_weekday(const local_days& dp) noexcept
      : year_month_weekday(from_days(dp.time_since_epoch().count())) {}
  template <class = void>
  constexpr year_month_weekday& operator+=(const months& m) noexcept;
  template <class = void>
  constexpr year_month_weekday& operator-=(const months& m) noexcept;
  constexpr year_month_weekday& operator+=(const years& y) noexcept {
    y_ += y;
    return *this;
  }
  constexpr year_month_weekday& operator-=(const years& y) noexcept {
    y_ -= y;
    return *this;
  }
  constexpr chrono::year year() const noexcept { return y_; }
  constexpr chrono::month month() const noexcept { return m_; }
  constexpr chrono::weekday weekday() const noexcept { return wdi_.weekday(); }
  constexpr unsigned index() const noexcept { return wdi_.index(); }
  constexpr chrono::weekday_indexed weekday_indexed() const noexcept { return wdi_; }
  constexpr operator sys_days() const noexcept { return sys_days(to_days()); }
  constexpr explicit operator local_days() const noexcept { return local_days(to_days()); }
  constexpr bool ok() const noexcept {
    if (!y_.ok() || !m_.ok() || !wdi_.ok())
      return false;
    if (wdi_.index() <= 4)
      return true;
    const int first = ::ycxx::detail::days_from_civil(static_cast<int>(y_), static_cast<unsigned>(m_), 1);
    const unsigned d = static_cast<unsigned>(to_days().count() - first) + 1;
    return d <= ::ycxx::detail::last_day_of(static_cast<int>(y_), static_cast<unsigned>(m_));
  }
};
constexpr bool operator==(const year_month_weekday& x, const year_month_weekday& y) noexcept {
  return x.year() == y.year() && x.month() == y.month() && x.weekday_indexed() == y.weekday_indexed();
}
template <class = void>
constexpr year_month_weekday operator+(const year_month_weekday& ymwd, const months& dm) noexcept {
  const year_month ym = year_month(ymwd.year(), ymwd.month()) + dm;
  return {ym.year(), ym.month(), ymwd.weekday_indexed()};
}
template <class = void>
constexpr year_month_weekday operator+(const months& dm, const year_month_weekday& ymwd) noexcept {
  return ymwd + dm;
}
template <class = void>
constexpr year_month_weekday operator-(const year_month_weekday& ymwd, const months& dm) noexcept {
  const year_month ym = ::ycxx::detail::add_months({ymwd.year(), ymwd.month()}, dm.count(), true);
  return {ym.year(), ym.month(), ymwd.weekday_indexed()};
}
constexpr year_month_weekday operator+(const year_month_weekday& ymwd, const years& dy) noexcept {
  return {ymwd.year() + dy, ymwd.month(), ymwd.weekday_indexed()};
}
constexpr year_month_weekday operator+(const years& dy, const year_month_weekday& ymwd) noexcept { return ymwd + dy; }
constexpr year_month_weekday operator-(const year_month_weekday& ymwd, const years& dy) noexcept {
  return {ymwd.year() - dy, ymwd.month(), ymwd.weekday_indexed()};
}
template <class V>
constexpr year_month_weekday& year_month_weekday::operator+=(const months& m) noexcept {
  *this = *this + m;
  return *this;
}
template <class V>
constexpr year_month_weekday& year_month_weekday::operator-=(const months& m) noexcept {
  *this = *this - m;
  return *this;
}

// [time.cal.ymwdlast]
class year_month_weekday_last {
  chrono::year y_;
  chrono::month m_;
  chrono::weekday_last wdl_;

  constexpr days to_days() const noexcept {
    const sys_days l = sys_days(year_month_day_last(y_, month_day_last(m_)));
    return l.time_since_epoch() - (chrono::weekday(l) - wdl_.weekday());
  }

public:
  constexpr year_month_weekday_last(const chrono::year& y, const chrono::month& m,
                                    const chrono::weekday_last& wdl) noexcept
      : y_(y), m_(m), wdl_(wdl) {}
  template <class = void>
  constexpr year_month_weekday_last& operator+=(const months& m) noexcept;
  template <class = void>
  constexpr year_month_weekday_last& operator-=(const months& m) noexcept;
  constexpr year_month_weekday_last& operator+=(const years& y) noexcept {
    y_ += y;
    return *this;
  }
  constexpr year_month_weekday_last& operator-=(const years& y) noexcept {
    y_ -= y;
    return *this;
  }
  constexpr chrono::year year() const noexcept { return y_; }
  constexpr chrono::month month() const noexcept { return m_; }
  constexpr chrono::weekday weekday() const noexcept { return wdl_.weekday(); }
  constexpr chrono::weekday_last weekday_last() const noexcept { return wdl_; }
  constexpr operator sys_days() const noexcept { return sys_days(to_days()); }
  constexpr explicit operator local_days() const noexcept { return local_days(to_days()); }
  constexpr bool ok() const noexcept { return y_.ok() && m_.ok() && wdl_.ok(); }
};
constexpr bool operator==(const year_month_weekday_last& x, const year_month_weekday_last& y) noexcept {
  return x.year() == y.year() && x.month() == y.month() && x.weekday_last() == y.weekday_last();
}
template <class = void>
constexpr year_month_weekday_last operator+(const year_month_weekday_last& ymwdl, const months& dm) noexcept {
  const year_month ym = year_month(ymwdl.year(), ymwdl.month()) + dm;
  return {ym.year(), ym.month(), ymwdl.weekday_last()};
}
template <class = void>
constexpr year_month_weekday_last operator+(const months& dm, const year_month_weekday_last& ymwdl) noexcept {
  return ymwdl + dm;
}
template <class = void>
constexpr year_month_weekday_last operator-(const year_month_weekday_last& ymwdl, const months& dm) noexcept {
  const year_month ym =
      ::ycxx::detail::add_months({ymwdl.year(), ymwdl.month()}, dm.count(), true);
  return {ym.year(), ym.month(), ymwdl.weekday_last()};
}
constexpr year_month_weekday_last operator+(const year_month_weekday_last& ymwdl, const years& dy) noexcept {
  return {ymwdl.year() + dy, ymwdl.month(), ymwdl.weekday_last()};
}
constexpr year_month_weekday_last operator+(const years& dy, const year_month_weekday_last& ymwdl) noexcept {
  return ymwdl + dy;
}
constexpr year_month_weekday_last operator-(const year_month_weekday_last& ymwdl, const years& dy) noexcept {
  return {ymwdl.year() - dy, ymwdl.month(), ymwdl.weekday_last()};
}
template <class V>
constexpr year_month_weekday_last& year_month_weekday_last::operator+=(const months& m) noexcept {
  *this = *this + m;
  return *this;
}
template <class V>
constexpr year_month_weekday_last& year_month_weekday_last::operator-=(const months& m) noexcept {
  *this = *this - m;
  return *this;
}

// [time.cal.operators]
constexpr year_month operator/(const year& y, const month& m) noexcept { return {y, m}; }
constexpr year_month operator/(const year& y, int m) noexcept { return {y, month(static_cast<unsigned>(m))}; }
constexpr month_day operator/(const month& m, const day& d) noexcept { return {m, d}; }
constexpr month_day operator/(const month& m, int d) noexcept { return {m, day(static_cast<unsigned>(d))}; }
constexpr month_day operator/(int m, const day& d) noexcept { return {month(static_cast<unsigned>(m)), d}; }
constexpr month_day operator/(const day& d, const month& m) noexcept { return {m, d}; }
constexpr month_day operator/(const day& d, int m) noexcept { return {month(static_cast<unsigned>(m)), d}; }
constexpr month_day_last operator/(const month& m, last_spec) noexcept { return month_day_last(m); }
constexpr month_day_last operator/(int m, last_spec) noexcept {
  return month_day_last(month(static_cast<unsigned>(m)));
}
constexpr month_day_last operator/(last_spec, const month& m) noexcept { return month_day_last(m); }
constexpr month_day_last operator/(last_spec, int m) noexcept {
  return month_day_last(month(static_cast<unsigned>(m)));
}
constexpr month_weekday operator/(const month& m, const weekday_indexed& wdi) noexcept { return {m, wdi}; }
constexpr month_weekday operator/(int m, const weekday_indexed& wdi) noexcept {
  return {month(static_cast<unsigned>(m)), wdi};
}
constexpr month_weekday operator/(const weekday_indexed& wdi, const month& m) noexcept { return {m, wdi}; }
constexpr month_weekday operator/(const weekday_indexed& wdi, int m) noexcept {
  return {month(static_cast<unsigned>(m)), wdi};
}
constexpr month_weekday_last operator/(const month& m, const weekday_last& wdl) noexcept { return {m, wdl}; }
constexpr month_weekday_last operator/(int m, const weekday_last& wdl) noexcept {
  return {month(static_cast<unsigned>(m)), wdl};
}
constexpr month_weekday_last operator/(const weekday_last& wdl, const month& m) noexcept { return {m, wdl}; }
constexpr month_weekday_last operator/(const weekday_last& wdl, int m) noexcept {
  return {month(static_cast<unsigned>(m)), wdl};
}
constexpr year_month_day operator/(const year_month& ym, const day& d) noexcept { return {ym.year(), ym.month(), d}; }
constexpr year_month_day operator/(const year_month& ym, int d) noexcept {
  return {ym.year(), ym.month(), day(static_cast<unsigned>(d))};
}
constexpr year_month_day operator/(const year& y, const month_day& md) noexcept { return {y, md.month(), md.day()}; }
constexpr year_month_day operator/(int y, const month_day& md) noexcept { return year(y) / md; }
constexpr year_month_day operator/(const month_day& md, const year& y) noexcept { return y / md; }
constexpr year_month_day operator/(const month_day& md, int y) noexcept { return year(y) / md; }
constexpr year_month_day_last operator/(const year_month& ym, last_spec) noexcept {
  return {ym.year(), month_day_last(ym.month())};
}
constexpr year_month_day_last operator/(const year& y, const month_day_last& mdl) noexcept { return {y, mdl}; }
constexpr year_month_day_last operator/(int y, const month_day_last& mdl) noexcept { return {year(y), mdl}; }
constexpr year_month_day_last operator/(const month_day_last& mdl, const year& y) noexcept { return {y, mdl}; }
constexpr year_month_day_last operator/(const month_day_last& mdl, int y) noexcept { return {year(y), mdl}; }
constexpr year_month_weekday operator/(const year_month& ym, const weekday_indexed& wdi) noexcept {
  return {ym.year(), ym.month(), wdi};
}
constexpr year_month_weekday operator/(const year& y, const month_weekday& mwd) noexcept {
  return {y, mwd.month(), mwd.weekday_indexed()};
}
constexpr year_month_weekday operator/(int y, const month_weekday& mwd) noexcept { return year(y) / mwd; }
constexpr year_month_weekday operator/(const month_weekday& mwd, const year& y) noexcept { return y / mwd; }
constexpr year_month_weekday operator/(const month_weekday& mwd, int y) noexcept { return year(y) / mwd; }
constexpr year_month_weekday_last operator/(const year_month& ym, const weekday_last& wdl) noexcept {
  return {ym.year(), ym.month(), wdl};
}
constexpr year_month_weekday_last operator/(const year& y, const month_weekday_last& mwdl) noexcept {
  return {y, mwdl.month(), mwdl.weekday_last()};
}
constexpr year_month_weekday_last operator/(int y, const month_weekday_last& mwdl) noexcept { return year(y) / mwdl; }
constexpr year_month_weekday_last operator/(const month_weekday_last& mwdl, const year& y) noexcept {
  return y / mwdl;
}
constexpr year_month_weekday_last operator/(const month_weekday_last& mwdl, int y) noexcept { return year(y) / mwdl; }

// Calendrical constants ([time.syn]).
inline constexpr weekday Sunday{0};
inline constexpr weekday Monday{1};
inline constexpr weekday Tuesday{2};
inline constexpr weekday Wednesday{3};
inline constexpr weekday Thursday{4};
inline constexpr weekday Friday{5};
inline constexpr weekday Saturday{6};
inline constexpr month January{1};
inline constexpr month February{2};
inline constexpr month March{3};
inline constexpr month April{4};
inline constexpr month May{5};
inline constexpr month June{6};
inline constexpr month July{7};
inline constexpr month August{8};
inline constexpr month September{9};
inline constexpr month October{10};
inline constexpr month November{11};
inline constexpr month December{12};

// [time.hms]
template <class Duration>
class hh_mm_ss {
  static_assert(ycxx::detail::is_duration<Duration>, "hh_mm_ss: Duration must be a specialization of duration");

public:
  static constexpr unsigned fractional_width = ycxx::detail::hms_fractional_width(Duration::period::den);
  using precision = chrono::duration<common_type_t<typename Duration::rep, seconds::rep>,
                                     ratio<1, ycxx::detail::pow10(fractional_width)>>;

private:
  bool neg_;
  chrono::hours h_;
  chrono::minutes m_;
  chrono::seconds s_;
  precision ss_;

  static constexpr Duration abs_of(Duration d) noexcept { return d < Duration::zero() ? -d : d; }

public:
  constexpr hh_mm_ss() noexcept : hh_mm_ss(Duration::zero()) {}
  constexpr explicit hh_mm_ss(Duration d)
      : neg_(d < Duration::zero()), h_(chrono::duration_cast<chrono::hours>(abs_of(d))),
        m_(chrono::duration_cast<chrono::minutes>(abs_of(d) - h_)),
        s_(chrono::duration_cast<chrono::seconds>(abs_of(d) - h_ - m_)), ss_() {
    if constexpr (treat_as_floating_point_v<typename precision::rep>)
      ss_ = abs_of(d) - h_ - m_ - s_;
    else
      ss_ = chrono::duration_cast<precision>(abs_of(d) - h_ - m_ - s_);
  }
  constexpr bool is_negative() const noexcept { return neg_; }
  constexpr chrono::hours hours() const noexcept { return h_; }
  constexpr chrono::minutes minutes() const noexcept { return m_; }
  constexpr chrono::seconds seconds() const noexcept { return s_; }
  constexpr precision subseconds() const noexcept { return ss_; }
  constexpr explicit operator precision() const noexcept { return to_duration(); }
  constexpr precision to_duration() const noexcept {
    const precision p = h_ + m_ + s_ + ss_;
    return neg_ ? -p : p;
  }
};

// [time.12]
constexpr bool is_am(const hours& h) noexcept { return hours(0) <= h && h <= hours(11); }
constexpr bool is_pm(const hours& h) noexcept { return hours(12) <= h && h <= hours(23); }
constexpr hours make12(const hours& h) noexcept {
  if (h == hours(0))
    return hours(12);
  return h > hours(12) ? h - hours(12) : h;
}
constexpr hours make24(const hours& h, bool is_pm) noexcept {
  if (is_pm)
    return h == hours(12) ? h : h + hours(12);
  return h == hours(12) ? hours(0) : h;
}

} // namespace std::chrono

namespace std {
inline namespace literals {
inline namespace chrono_literals {
// [time.cal.day.nonmembers], [time.cal.year.nonmembers]
constexpr chrono::day operator""d(unsigned long long d) noexcept { return chrono::day(static_cast<unsigned>(d)); }
constexpr chrono::year operator""y(unsigned long long y) noexcept { return chrono::year(static_cast<int>(y)); }
} // namespace chrono_literals
} // namespace literals

// [time.hash]/3: enabled for every value, ok() or not.
template <>
struct hash<chrono::day> {
  size_t operator()(const chrono::day& d) const noexcept { return static_cast<unsigned>(d); }
};
template <>
struct hash<chrono::month> {
  size_t operator()(const chrono::month& m) const noexcept { return static_cast<unsigned>(m); }
};
template <>
struct hash<chrono::year> {
  size_t operator()(const chrono::year& y) const noexcept {
    return static_cast<size_t>(static_cast<unsigned short>(static_cast<int>(y)));
  }
};
template <>
struct hash<chrono::weekday> {
  size_t operator()(const chrono::weekday& w) const noexcept { return w.c_encoding(); }
};
template <>
struct hash<chrono::weekday_indexed> {
  size_t operator()(const chrono::weekday_indexed& w) const noexcept {
    return w.weekday().c_encoding() << 8 | (w.index() & 0xff);
  }
};
template <>
struct hash<chrono::weekday_last> {
  size_t operator()(const chrono::weekday_last& w) const noexcept { return w.weekday().c_encoding(); }
};
template <>
struct hash<chrono::month_day> {
  size_t operator()(const chrono::month_day& md) const noexcept {
    return static_cast<unsigned>(md.month()) << 8 | static_cast<unsigned>(md.day());
  }
};
template <>
struct hash<chrono::month_day_last> {
  size_t operator()(const chrono::month_day_last& mdl) const noexcept { return static_cast<unsigned>(mdl.month()); }
};
template <>
struct hash<chrono::month_weekday> {
  size_t operator()(const chrono::month_weekday& mwd) const noexcept {
    return static_cast<unsigned>(mwd.month()) << 16 | hash<chrono::weekday_indexed>{}(mwd.weekday_indexed());
  }
};
template <>
struct hash<chrono::month_weekday_last> {
  size_t operator()(const chrono::month_weekday_last& mwdl) const noexcept {
    return static_cast<unsigned>(mwdl.month()) << 8 | mwdl.weekday_last().weekday().c_encoding();
  }
};
template <>
struct hash<chrono::year_month> {
  size_t operator()(const chrono::year_month& ym) const noexcept {
    return hash<chrono::year>{}(ym.year()) << 8 | static_cast<unsigned>(ym.month());
  }
};
template <>
struct hash<chrono::year_month_day> {
  size_t operator()(const chrono::year_month_day& ymd) const noexcept {
    return hash<chrono::year>{}(ymd.year()) << 16 | static_cast<unsigned>(ymd.month()) << 8 |
           static_cast<unsigned>(ymd.day());
  }
};
template <>
struct hash<chrono::year_month_day_last> {
  size_t operator()(const chrono::year_month_day_last& ymdl) const noexcept {
    return hash<chrono::year>{}(ymdl.year()) << 8 | static_cast<unsigned>(ymdl.month());
  }
};
template <>
struct hash<chrono::year_month_weekday> {
  size_t operator()(const chrono::year_month_weekday& ymwd) const noexcept {
    return hash<chrono::year>{}(ymwd.year()) << 24 | static_cast<unsigned>(ymwd.month()) << 16 |
           hash<chrono::weekday_indexed>{}(ymwd.weekday_indexed());
  }
};
template <>
struct hash<chrono::year_month_weekday_last> {
  size_t operator()(const chrono::year_month_weekday_last& ymwdl) const noexcept {
    return hash<chrono::year>{}(ymwdl.year()) << 16 | static_cast<unsigned>(ymwdl.month()) << 8 |
           ymwdl.weekday().c_encoding();
  }
};

} // namespace std
