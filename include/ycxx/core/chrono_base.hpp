// libycxx core: the time-arithmetic part of <chrono> ([time.traits], [time.duration],
// [time.point], [time.duration.literals], [time.duration.alg], the duration and time_point parts
// of [time.hash]), and the clock-independent aliases (sys_time, local_t, local_time, file_time).
//
// The clocks themselves need the OS and are hosted (ycxx/hosted/chrono_clocks.hpp); here they
// are only declared. Calendars, time zones, formatting and parsing are not part of this file.
//
// The integer duration literals (24h, 5ms, ...) are consteval: [time.duration.literals]/3 makes a
// literal whose value does not fit the result type ill-formed, and only an immediate function
// can diagnose that for a literal used to initialize a variable with dynamic initialization.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/ratio.hpp>
#include <ycxx/core/type_traits.hpp>

namespace std::chrono {
template <class Rep, class Period = ratio<1>>
class duration;
template <class Clock, class Duration = typename Clock::duration>
class time_point;
} // namespace std::chrono

namespace ycxx::detail {

template <class T>
inline constexpr bool is_duration = false;
template <class Rep, class Period>
inline constexpr bool is_duration<std::chrono::duration<Rep, Period>> = true;
template <class T>
concept duration_type = is_duration<T>;

// ratio_divide<P1, P2> is a valid ratio specialization (no overflow): the converting constructor
// of duration is constrained on it ([time.duration.cons]/3).
template <class P1, class P2>
concept ratio_divide_valid = ::ycxx::detail::ratio_div(P1::num, P1::den, P2::num, P2::den).ok;

template <std::intmax_t A, std::intmax_t B>
inline constexpr std::intmax_t static_gcd = ::ycxx::detail::ratio_gcd(A, B);

// [time.traits.specializations]/1: gcd(N1, N2) / lcm(D1, D2).
template <class P1, class P2>
using ratio_gcd_t = std::ratio<static_gcd<P1::num, P2::num>, (P1::den / static_gcd<P1::den, P2::den>) * P2::den>;

} // namespace ycxx::detail

namespace std {

// [time.traits.specializations]
template <class Rep1, class Period1, class Rep2, class Period2>
  requires requires { typename common_type_t<Rep1, Rep2>; }
struct common_type<chrono::duration<Rep1, Period1>, chrono::duration<Rep2, Period2>> {
  using type = chrono::duration<common_type_t<Rep1, Rep2>,
                                ycxx::detail::ratio_gcd_t<typename Period1::type, typename Period2::type>>;
};
template <class Clock, class Duration1, class Duration2>
  requires requires { typename common_type_t<Duration1, Duration2>; }
struct common_type<chrono::time_point<Clock, Duration1>, chrono::time_point<Clock, Duration2>> {
  using type = chrono::time_point<Clock, common_type_t<Duration1, Duration2>>;
};

} // namespace std

namespace std::chrono {

// [time.traits.is.fp]
template <class Rep>
struct treat_as_floating_point : is_floating_point<Rep> {};
template <class Rep>
constexpr bool treat_as_floating_point_v = treat_as_floating_point<Rep>::value;

// [time.traits.duration.values]
template <class Rep>
struct duration_values {
  static constexpr Rep zero() noexcept { return Rep(0); }
  static constexpr Rep min() noexcept { return numeric_limits<Rep>::lowest(); }
  static constexpr Rep max() noexcept { return numeric_limits<Rep>::max(); }
};

// [time.traits.is.clock]
template <class T>
struct is_clock : bool_constant<requires {
  typename T::rep;
  typename T::period;
  typename T::duration;
  typename T::time_point;
  T::is_steady;
  T::now();
}> {};
template <class T>
constexpr bool is_clock_v = is_clock<T>::value;

// [time.duration.cast]
template <class ToDuration, class Rep, class Period>
  requires ycxx::detail::is_duration<ToDuration>
constexpr ToDuration duration_cast(const duration<Rep, Period>& d) {
  using cf = ratio_divide<Period, typename ToDuration::period>;
  using cr = common_type_t<typename ToDuration::rep, Rep, intmax_t>;
  using to_rep = typename ToDuration::rep;
  if constexpr (cf::num == 1 && cf::den == 1)
    return ToDuration(static_cast<to_rep>(d.count()));
  else if constexpr (cf::den == 1)
    return ToDuration(static_cast<to_rep>(static_cast<cr>(d.count()) * static_cast<cr>(cf::num)));
  else if constexpr (cf::num == 1)
    return ToDuration(static_cast<to_rep>(static_cast<cr>(d.count()) / static_cast<cr>(cf::den)));
  else
    return ToDuration(
        static_cast<to_rep>(static_cast<cr>(d.count()) * static_cast<cr>(cf::num) / static_cast<cr>(cf::den)));
}

// [time.duration]
template <class Rep, class Period>
class duration {
  static_assert(!ycxx::detail::is_duration<Rep>, "duration: Rep must not be a duration ([time.duration.general]/2)");
  static_assert(!is_const_v<Rep> && !is_volatile_v<Rep>, "duration: Rep must not be cv-qualified");
  static_assert(ycxx::detail::is_ratio<Period>, "duration: Period must be a specialization of ratio");
  static_assert(Period::num > 0, "duration: Period must be positive");

public:
  using rep = Rep;
  using period = typename Period::type;

private:
  rep rep_;

public:
  constexpr duration() = default;
  template <class Rep2>
    requires is_convertible_v<const Rep2&, rep> &&
             (treat_as_floating_point_v<rep> || !treat_as_floating_point_v<Rep2>)
  constexpr explicit duration(const Rep2& r) : rep_(r) {}
  template <class Rep2, class Period2>
    requires is_convertible_v<const Rep2&, rep> && ycxx::detail::ratio_divide_valid<typename Period2::type, period> &&
             (treat_as_floating_point_v<rep> ||
              (ratio_divide<typename Period2::type, period>::den == 1 && !treat_as_floating_point_v<Rep2>))
  constexpr duration(const duration<Rep2, Period2>& d) : rep_(chrono::duration_cast<duration>(d).count()) {}
  ~duration() = default;
  duration(const duration&) = default;
  duration& operator=(const duration&) = default;

  constexpr rep count() const { return rep_; }

  constexpr common_type_t<duration> operator+() const { return common_type_t<duration>(*this); }
  constexpr common_type_t<duration> operator-() const { return common_type_t<duration>(-rep_); }
  constexpr duration& operator++() {
    ++rep_;
    return *this;
  }
  constexpr duration operator++(int) { return duration(rep_++); }
  constexpr duration& operator--() {
    --rep_;
    return *this;
  }
  constexpr duration operator--(int) { return duration(rep_--); }
  constexpr duration& operator+=(const duration& d) {
    rep_ += d.count();
    return *this;
  }
  constexpr duration& operator-=(const duration& d) {
    rep_ -= d.count();
    return *this;
  }
  constexpr duration& operator*=(const rep& rhs) {
    rep_ *= rhs;
    return *this;
  }
  constexpr duration& operator/=(const rep& rhs) {
    rep_ /= rhs;
    return *this;
  }
  constexpr duration& operator%=(const rep& rhs) {
    rep_ %= rhs;
    return *this;
  }
  constexpr duration& operator%=(const duration& rhs) {
    rep_ %= rhs.count();
    return *this;
  }

  static constexpr duration zero() noexcept { return duration(duration_values<rep>::zero()); }
  static constexpr duration min() noexcept { return duration(duration_values<rep>::min()); }
  static constexpr duration max() noexcept { return duration(duration_values<rep>::max()); }
};

using nanoseconds = duration<long long, nano>;
using microseconds = duration<long long, micro>;
using milliseconds = duration<long long, milli>;
using seconds = duration<long long>;
using minutes = duration<long long, ratio<60>>;
using hours = duration<long long, ratio<3600>>;
using days = duration<int, ratio_multiply<ratio<24>, hours::period>>;
using weeks = duration<int, ratio_multiply<ratio<7>, days::period>>;
using years = duration<int, ratio_multiply<ratio<146097, 400>, days::period>>;
using months = duration<int, ratio_divide<years::period, ratio<12>>>;

// [time.duration.nonmember]
template <class Rep1, class Period1, class Rep2, class Period2>
constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>> operator+(const duration<Rep1, Period1>& lhs,
                                                                                   const duration<Rep2, Period2>& rhs) {
  using cd = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
  return cd(cd(lhs).count() + cd(rhs).count());
}
template <class Rep1, class Period1, class Rep2, class Period2>
constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>> operator-(const duration<Rep1, Period1>& lhs,
                                                                                   const duration<Rep2, Period2>& rhs) {
  using cd = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
  return cd(cd(lhs).count() - cd(rhs).count());
}
template <class Rep1, class Period, class Rep2>
  requires is_convertible_v<const Rep2&, common_type_t<Rep1, Rep2>>
constexpr duration<common_type_t<Rep1, Rep2>, Period> operator*(const duration<Rep1, Period>& d, const Rep2& s) {
  using cd = duration<common_type_t<Rep1, Rep2>, Period>;
  return cd(cd(d).count() * s);
}
template <class Rep1, class Rep2, class Period>
  requires is_convertible_v<const Rep1&, common_type_t<Rep1, Rep2>>
constexpr duration<common_type_t<Rep1, Rep2>, Period> operator*(const Rep1& s, const duration<Rep2, Period>& d) {
  return d * s;
}
template <class Rep1, class Period, class Rep2>
  requires(!ycxx::detail::is_duration<Rep2>) && is_convertible_v<const Rep2&, common_type_t<Rep1, Rep2>>
constexpr duration<common_type_t<Rep1, Rep2>, Period> operator/(const duration<Rep1, Period>& d, const Rep2& s) {
  using cd = duration<common_type_t<Rep1, Rep2>, Period>;
  return cd(cd(d).count() / s);
}
template <class Rep1, class Period1, class Rep2, class Period2>
constexpr common_type_t<Rep1, Rep2> operator/(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
  using cd = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
  return cd(lhs).count() / cd(rhs).count();
}
template <class Rep1, class Period, class Rep2>
  requires(!ycxx::detail::is_duration<Rep2>) && is_convertible_v<const Rep2&, common_type_t<Rep1, Rep2>>
constexpr duration<common_type_t<Rep1, Rep2>, Period> operator%(const duration<Rep1, Period>& d, const Rep2& s) {
  using cd = duration<common_type_t<Rep1, Rep2>, Period>;
  return cd(cd(d).count() % s);
}
template <class Rep1, class Period1, class Rep2, class Period2>
constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>> operator%(const duration<Rep1, Period1>& lhs,
                                                                                   const duration<Rep2, Period2>& rhs) {
  using cd = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
  return cd(cd(lhs).count() % cd(rhs).count());
}

// [time.duration.comparisons]
template <class Rep1, class Period1, class Rep2, class Period2>
constexpr bool operator==(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
  using ct = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
  return ct(lhs).count() == ct(rhs).count();
}
template <class Rep1, class Period1, class Rep2, class Period2>
constexpr bool operator<(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
  using ct = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
  return ct(lhs).count() < ct(rhs).count();
}
template <class Rep1, class Period1, class Rep2, class Period2>
constexpr bool operator>(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
  return rhs < lhs;
}
template <class Rep1, class Period1, class Rep2, class Period2>
constexpr bool operator<=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
  return !(rhs < lhs);
}
template <class Rep1, class Period1, class Rep2, class Period2>
constexpr bool operator>=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
  return !(lhs < rhs);
}
template <class Rep1, class Period1, class Rep2, class Period2>
  requires three_way_comparable<typename common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>::rep>
constexpr auto operator<=>(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
  using ct = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>>;
  return ct(lhs).count() <=> ct(rhs).count();
}

// [time.duration.cast]
template <class ToDuration, class Rep, class Period>
  requires ycxx::detail::is_duration<ToDuration>
constexpr ToDuration floor(const duration<Rep, Period>& d) {
  ToDuration t = chrono::duration_cast<ToDuration>(d);
  if (t > d)
    return t - ToDuration{1};
  return t;
}
template <class ToDuration, class Rep, class Period>
  requires ycxx::detail::is_duration<ToDuration>
constexpr ToDuration ceil(const duration<Rep, Period>& d) {
  ToDuration t = chrono::duration_cast<ToDuration>(d);
  if (t < d)
    return t + ToDuration{1};
  return t;
}
template <class ToDuration, class Rep, class Period>
  requires ycxx::detail::is_duration<ToDuration> && (!treat_as_floating_point_v<typename ToDuration::rep>)
constexpr ToDuration round(const duration<Rep, Period>& d) {
  ToDuration t0 = chrono::floor<ToDuration>(d);
  ToDuration t1 = t0 + ToDuration{1};
  auto diff0 = d - t0;
  auto diff1 = t1 - d;
  if (diff0 == diff1)
    return (t0.count() & 1) ? t1 : t0;
  return diff0 < diff1 ? t0 : t1;
}

// [time.duration.alg]
template <class Rep, class Period>
  requires numeric_limits<Rep>::is_signed
constexpr duration<Rep, Period> abs(duration<Rep, Period> d) {
  return d >= d.zero() ? d : static_cast<duration<Rep, Period>>(-d);
}

// [time.point]
template <class Clock, class Duration>
class time_point {
  static_assert(ycxx::detail::is_duration<Duration>, "time_point: Duration must be a specialization of duration");

public:
  using clock = Clock;
  using duration = Duration;
  using rep = typename duration::rep;
  using period = typename duration::period;

private:
  duration d_;

public:
  constexpr time_point() : d_(duration::zero()) {}
  constexpr explicit time_point(const duration& d) : d_(d) {}
  template <class Duration2>
    requires is_convertible_v<Duration2, duration>
  constexpr time_point(const time_point<clock, Duration2>& t) : d_(t.time_since_epoch()) {}

  constexpr duration time_since_epoch() const { return d_; }

  constexpr time_point& operator++() {
    ++d_;
    return *this;
  }
  constexpr time_point operator++(int) { return time_point{d_++}; }
  constexpr time_point& operator--() {
    --d_;
    return *this;
  }
  constexpr time_point operator--(int) { return time_point{d_--}; }
  constexpr time_point& operator+=(const duration& d) {
    d_ += d;
    return *this;
  }
  constexpr time_point& operator-=(const duration& d) {
    d_ -= d;
    return *this;
  }

  static constexpr time_point min() noexcept { return time_point(duration::min()); }
  static constexpr time_point max() noexcept { return time_point(duration::max()); }
};

// [time.point.nonmember]
template <class Clock, class Duration1, class Rep2, class Period2>
constexpr time_point<Clock, common_type_t<Duration1, duration<Rep2, Period2>>>
operator+(const time_point<Clock, Duration1>& lhs, const duration<Rep2, Period2>& rhs) {
  using ct = time_point<Clock, common_type_t<Duration1, duration<Rep2, Period2>>>;
  return ct(lhs.time_since_epoch() + rhs);
}
template <class Rep1, class Period1, class Clock, class Duration2>
constexpr time_point<Clock, common_type_t<duration<Rep1, Period1>, Duration2>>
operator+(const duration<Rep1, Period1>& lhs, const time_point<Clock, Duration2>& rhs) {
  return rhs + lhs;
}
template <class Clock, class Duration1, class Rep2, class Period2>
constexpr time_point<Clock, common_type_t<Duration1, duration<Rep2, Period2>>>
operator-(const time_point<Clock, Duration1>& lhs, const duration<Rep2, Period2>& rhs) {
  using ct = time_point<Clock, common_type_t<Duration1, duration<Rep2, Period2>>>;
  return ct(lhs.time_since_epoch() - rhs);
}
template <class Clock, class Duration1, class Duration2>
constexpr common_type_t<Duration1, Duration2> operator-(const time_point<Clock, Duration1>& lhs,
                                                        const time_point<Clock, Duration2>& rhs) {
  return lhs.time_since_epoch() - rhs.time_since_epoch();
}

// [time.point.comparisons]
template <class Clock, class Duration1, class Duration2>
constexpr bool operator==(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs) {
  return lhs.time_since_epoch() == rhs.time_since_epoch();
}
template <class Clock, class Duration1, class Duration2>
constexpr bool operator<(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs) {
  return lhs.time_since_epoch() < rhs.time_since_epoch();
}
template <class Clock, class Duration1, class Duration2>
constexpr bool operator>(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs) {
  return rhs < lhs;
}
template <class Clock, class Duration1, class Duration2>
constexpr bool operator<=(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs) {
  return !(rhs < lhs);
}
template <class Clock, class Duration1, class Duration2>
constexpr bool operator>=(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs) {
  return !(lhs < rhs);
}
template <class Clock, class Duration1, three_way_comparable_with<Duration1> Duration2>
constexpr auto operator<=>(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs) {
  return lhs.time_since_epoch() <=> rhs.time_since_epoch();
}

// [time.point.cast]
template <class ToDuration, class Clock, class Duration>
  requires ycxx::detail::is_duration<ToDuration>
constexpr time_point<Clock, ToDuration> time_point_cast(const time_point<Clock, Duration>& t) {
  return time_point<Clock, ToDuration>(chrono::duration_cast<ToDuration>(t.time_since_epoch()));
}
template <class ToDuration, class Clock, class Duration>
  requires ycxx::detail::is_duration<ToDuration>
constexpr time_point<Clock, ToDuration> floor(const time_point<Clock, Duration>& tp) {
  return time_point<Clock, ToDuration>(chrono::floor<ToDuration>(tp.time_since_epoch()));
}
template <class ToDuration, class Clock, class Duration>
  requires ycxx::detail::is_duration<ToDuration>
constexpr time_point<Clock, ToDuration> ceil(const time_point<Clock, Duration>& tp) {
  return time_point<Clock, ToDuration>(chrono::ceil<ToDuration>(tp.time_since_epoch()));
}
template <class ToDuration, class Clock, class Duration>
  requires ycxx::detail::is_duration<ToDuration> && (!treat_as_floating_point_v<typename ToDuration::rep>)
constexpr time_point<Clock, ToDuration> round(const time_point<Clock, Duration>& tp) {
  return time_point<Clock, ToDuration>(chrono::round<ToDuration>(tp.time_since_epoch()));
}

// Clocks: declared here, defined in ycxx/hosted/chrono_clocks.hpp.
class system_clock;
class steady_clock;
class high_resolution_clock;

template <class Duration>
using sys_time = time_point<system_clock, Duration>;
using sys_seconds = sys_time<seconds>;
using sys_days = sys_time<days>;

// [time.clock.local]
struct local_t {};
template <class Duration>
using local_time = time_point<local_t, Duration>;
using local_seconds = local_time<seconds>;
using local_days = local_time<days>;

} // namespace std::chrono

namespace ycxx::adl_free {
// [time.clock.file]: the type std::chrono::file_clock denotes (defined with the other clocks).
class file_clock;
} // namespace ycxx::adl_free

namespace std::chrono {
using file_clock = ycxx::adl_free::file_clock;
template <class Duration>
using file_time = time_point<file_clock, Duration>;
} // namespace std::chrono

namespace ycxx::detail {
// Diagnoses an integer duration literal that overflows its type ([time.duration.literals]/3):
// not constexpr, so reaching it in the immediate literal operators is ill-formed.
inline void duration_literal_overflows() noexcept {}
template <class D>
consteval D duration_literal(unsigned long long v) {
  if (v > static_cast<unsigned long long>(std::numeric_limits<typename D::rep>::max()))
    ::ycxx::detail::duration_literal_overflows();
  return D(static_cast<typename D::rep>(v));
}
} // namespace ycxx::detail

namespace std {
inline namespace literals {
inline namespace chrono_literals {
// [time.duration.literals]
consteval chrono::hours operator""h(unsigned long long v) {
  return ycxx::detail::duration_literal<chrono::hours>(v);
}
constexpr chrono::duration<long double, ratio<3600>> operator""h(long double v) {
  return chrono::duration<long double, ratio<3600>>(v);
}
consteval chrono::minutes operator""min(unsigned long long v) {
  return ycxx::detail::duration_literal<chrono::minutes>(v);
}
constexpr chrono::duration<long double, ratio<60>> operator""min(long double v) {
  return chrono::duration<long double, ratio<60>>(v);
}
consteval chrono::seconds operator""s(unsigned long long v) {
  return ycxx::detail::duration_literal<chrono::seconds>(v);
}
constexpr chrono::duration<long double> operator""s(long double v) { return chrono::duration<long double>(v); }
consteval chrono::milliseconds operator""ms(unsigned long long v) {
  return ycxx::detail::duration_literal<chrono::milliseconds>(v);
}
constexpr chrono::duration<long double, milli> operator""ms(long double v) {
  return chrono::duration<long double, milli>(v);
}
consteval chrono::microseconds operator""us(unsigned long long v) {
  return ycxx::detail::duration_literal<chrono::microseconds>(v);
}
constexpr chrono::duration<long double, micro> operator""us(long double v) {
  return chrono::duration<long double, micro>(v);
}
consteval chrono::nanoseconds operator""ns(unsigned long long v) {
  return ycxx::detail::duration_literal<chrono::nanoseconds>(v);
}
constexpr chrono::duration<long double, nano> operator""ns(long double v) {
  return chrono::duration<long double, nano>(v);
}
} // namespace chrono_literals
} // namespace literals

namespace chrono {
using namespace literals::chrono_literals;
} // namespace chrono

// [time.hash]
template <class Rep, class Period>
  requires ycxx::detail::hash_enabled<Rep>
struct hash<chrono::duration<Rep, Period>> {
  size_t operator()(const chrono::duration<Rep, Period>& d) const { return hash<Rep>{}(d.count()); }
};
template <class Clock, class Duration>
  requires ycxx::detail::hash_enabled<Duration>
struct hash<chrono::time_point<Clock, Duration>> {
  size_t operator()(const chrono::time_point<Clock, Duration>& t) const {
    return hash<Duration>{}(t.time_since_epoch());
  }
};

} // namespace std
