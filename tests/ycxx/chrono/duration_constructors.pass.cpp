// [time.duration.cons]: duration(const Rep2& r) requires is_convertible_v<const Rep2&, rep> and
// (treat_as_floating_point_v<rep> or !treat_as_floating_point_v<Rep2>); it is explicit.
// duration(const duration<Rep2, Period2>&) requires convertibility and either a floating rep or an
// exact (den == 1) conversion from a non-floating Rep2; it initializes rep_ with
// duration_cast<duration>(d).count().
#include <chrono>
#include <ratio>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

using ms_i = duration<int, std::milli>;
using us_i = duration<int, std::micro>;
using s_d = duration<double>;
using ms_d = duration<double, std::milli>;

// From a representation value.
static_assert(std::is_constructible_v<ms_i, int>);
static_assert(std::is_constructible_v<ms_i, long long>);
static_assert(!std::is_constructible_v<ms_i, double>);  // "duration<int, milli> d2(3.5); // error"
static_assert(std::is_constructible_v<s_d, int>);
static_assert(std::is_constructible_v<s_d, double>);
static_assert(!std::is_convertible_v<int, ms_i>);  // explicit
static_assert(!std::is_constructible_v<ms_i, int*>);

// From another duration.
static_assert(std::is_convertible_v<ms_i, us_i>);   // "duration<int, micro> us = ms; // OK"
static_assert(!std::is_convertible_v<us_i, ms_i>);  // "duration<int, milli> ms2 = us; // error"
static_assert(!std::is_constructible_v<ms_i, us_i>);
static_assert(std::is_convertible_v<hours, seconds>);
static_assert(!std::is_convertible_v<seconds, minutes>);
static_assert(std::is_convertible_v<us_i, ms_d>);    // floating destination: always
static_assert(std::is_convertible_v<s_d, ms_d>);
static_assert(!std::is_convertible_v<s_d, ms_i>);    // floating source, integral destination
static_assert(!std::is_convertible_v<ms_d, seconds>);
static_assert(std::is_convertible_v<duration<int, std::ratio<1, 2>>, duration<int, std::ratio<1, 4>>>);
static_assert(!std::is_convertible_v<duration<int, std::ratio<1, 3>>, duration<int, std::ratio<1, 2>>>);
static_assert(std::is_convertible_v<years, seconds>);  // 31556952 s per year
static_assert(!std::is_convertible_v<years, days>);    // 146097/400 days is not integral
static_assert(std::is_convertible_v<weeks, days> && !std::is_convertible_v<days, weeks>);

constexpr bool test() {
  ms_i a(3);
  if (a.count() != 3) return false;
  us_i b = a;
  if (b.count() != 3000) return false;
  s_d c(2);
  if (c.count() != 2.0) return false;
  ms_d d = s_d(1.5);
  if (d.count() != 1500.0) return false;
  ms_d e = us_i(2500);
  if (e.count() != 2.5) return false;
  seconds f = hours(2);
  if (f.count() != 7200) return false;
  duration<long long, std::ratio<1, 4>> g = duration<int, std::ratio<1, 2>>(3);
  if (g.count() != 6) return false;
  seconds h{};  // value-initialized: zero
  if (h.count() != 0) return false;
  seconds i = minutes(-2);
  if (i.count() != -120) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
