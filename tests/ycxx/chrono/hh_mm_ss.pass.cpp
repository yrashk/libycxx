// [time.hms.members]: hh_mm_ss(d) sets is-neg to d < 0 and splits ABS_D into hours, minutes,
// seconds (duration_cast) and subseconds (ABS_D minus those, duration_cast<precision> unless
// precision::rep is floating); the fields are non-negative; to_duration() is
// +-(h + m + s + ss) and equals duration_cast<precision>(d) (or d for floating precision);
// explicit operator precision; the default constructor represents zero.
#include <chrono>
#include <ratio>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(!std::is_convertible_v<seconds, hh_mm_ss<seconds>>);
static_assert(std::is_constructible_v<hh_mm_ss<seconds>, seconds>);
static_assert(!std::is_convertible_v<hh_mm_ss<seconds>, seconds>);
static_assert(std::is_nothrow_default_constructible_v<hh_mm_ss<seconds>>);
static_assert(noexcept(std::declval<hh_mm_ss<seconds>>().hours()) && noexcept(std::declval<hh_mm_ss<seconds>>().to_duration()));
static_assert(std::is_same_v<decltype(std::declval<hh_mm_ss<milliseconds>>().hours()), hours>);
static_assert(std::is_same_v<decltype(std::declval<hh_mm_ss<milliseconds>>().minutes()), minutes>);
static_assert(std::is_same_v<decltype(std::declval<hh_mm_ss<milliseconds>>().seconds()), seconds>);
static_assert(std::is_same_v<decltype(std::declval<hh_mm_ss<milliseconds>>().subseconds()),
                             hh_mm_ss<milliseconds>::precision>);

constexpr bool test() {
  hh_mm_ss<seconds> a(seconds(3661));
  if (a.is_negative() || a.hours() != hours(1) || a.minutes() != minutes(1) || a.seconds() != seconds(1)) return false;
  if (a.subseconds() != seconds(0) || a.to_duration() != seconds(3661)) return false;
  hh_mm_ss<seconds> b(seconds(-3661));
  if (!b.is_negative() || b.hours() != hours(1) || b.minutes() != minutes(1) || b.seconds() != seconds(1)) return false;
  if (b.to_duration() != seconds(-3661) || seconds(b) != seconds(-3661)) return false;
  hh_mm_ss<milliseconds> c(milliseconds(-90061001));  // -(25h 1min 1s 1ms)
  if (!c.is_negative() || c.hours() != hours(25) || c.minutes() != minutes(1) || c.seconds() != seconds(1)) return false;
  if (c.subseconds() != milliseconds(1) || c.to_duration() != milliseconds(-90061001)) return false;
  hh_mm_ss<minutes> d(minutes(125));
  if (d.hours() != hours(2) || d.minutes() != minutes(5) || d.seconds() != seconds(0)) return false;
  if (d.subseconds().count() != 0) return false;
  hh_mm_ss<hours> e(days(2) + hours(1));
  if (e.hours() != hours(49)) return false;
  hh_mm_ss<seconds> zero;
  if (zero.is_negative() || zero.to_duration() != seconds(0) || zero.hours() != hours(0)) return false;
  hh_mm_ss<seconds> negzero(seconds(0));
  if (negzero.is_negative()) return false;
  // Non-decimal periods: precision is a power-of-ten fraction.
  hh_mm_ss<duration<int, std::ratio<1, 3>>> f(duration<int, std::ratio<1, 3>>(4));  // 4/3 s
  if (f.seconds() != seconds(1) || f.subseconds().count() != 333333) return false;
  if (f.to_duration().count() != 1333333) return false;  // duration_cast<precision>(d)
  hh_mm_ss<duration<int, std::ratio<1, 4>>> g(duration<int, std::ratio<1, 4>>(-7));  // -1.75 s
  if (!g.is_negative() || g.seconds() != seconds(1) || g.subseconds().count() != 75) return false;
  // Coarser than a second but not a whole number of minutes.
  hh_mm_ss<duration<int, std::ratio<90>>> h(duration<int, std::ratio<90>>(3));  // 270 s
  if (h.minutes() != minutes(4) || h.seconds() != seconds(30)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // Floating-point representation: subseconds is computed without duration_cast.
  hh_mm_ss<duration<double>> f(duration<double>(3723.25));
  CHECK(f.hours() == hours(1) && f.minutes() == minutes(2) && f.seconds() == seconds(3));
  CHECK(f.subseconds().count() == 0.25);
  CHECK(f.to_duration().count() == 3723.25);
  hh_mm_ss<duration<double, std::milli>> g(duration<double, std::milli>(-1500.5));
  CHECK(g.is_negative() && g.seconds() == seconds(1));
  CHECK(g.to_duration().count() == -1500.5);
}
