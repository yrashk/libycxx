// [time.duration.cast]/2: with CF = ratio_divide<Period, ToDuration::period> and
// CR = common_type<ToDuration::rep, Rep, intmax_t>::type, duration_cast computes
// static_cast<ToRep>(static_cast<CR>(d.count()) * CF::num / CF::den) (dropping the * or / when
// CF::num or CF::den is 1); intermediate values are in CR. Constraints: ToDuration is a
// specialization of duration.
#include <chrono>
#include <cstdint>
#include <ratio>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using std::ratio;

template <class To, class From>
concept castable = requires(From f) { duration_cast<To>(f); };
static_assert(castable<seconds, milliseconds> && castable<duration<double>, seconds>);
static_assert(!castable<int, seconds> && !castable<sys_seconds, seconds>);
static_assert(std::is_same_v<decltype(duration_cast<minutes>(seconds(1))), minutes>);

constexpr bool test() {
  if (duration_cast<seconds>(milliseconds(1999)).count() != 1) return false;   // truncation
  if (duration_cast<seconds>(milliseconds(-1999)).count() != -1) return false; // towards zero
  if (duration_cast<milliseconds>(seconds(3)).count() != 3000) return false;
  if (duration_cast<minutes>(hours(-2)).count() != -120) return false;
  if (duration_cast<seconds>(seconds(42)).count() != 42) return false;
  // CF = 3/2: 5 * 3 / 2 = 7.
  if (duration_cast<duration<int, ratio<1, 3>>>(duration<int, ratio<1, 2>>(5)).count() != 7) return false;
  // Intermediate computation in intmax_t: 10^9 * 5 overflows int but not the result.
  if (duration_cast<duration<int, ratio<1, 5>>>(duration<int, ratio<1, 3>>(1000000000)).count() != 1666666666)
    return false;
  if (duration_cast<duration<int, std::milli>>(duration<int, std::micro>(2147483647)).count() != 2147483) return false;
  // Floating point.
  if (duration_cast<duration<double>>(milliseconds(1)).count() != 0.001) return false;
  if (duration_cast<seconds>(duration<double>(2.9)).count() != 2) return false;
  if (duration_cast<seconds>(duration<double>(-2.9)).count() != -2) return false;
  if (duration_cast<duration<double, std::milli>>(duration<double>(1.25)).count() != 1250.0) return false;
  if (duration_cast<duration<float>>(minutes(1)).count() != 60.0f) return false;
  // Unsigned and narrow representations.
  if (duration_cast<duration<unsigned, std::milli>>(seconds(4)).count() != 4000u) return false;
  if (duration_cast<duration<short>>(milliseconds(32767000)).count() != 32767) return false;
  // Calendar periods.
  if (duration_cast<days>(years(400)).count() != 146097) return false;
  if (duration_cast<months>(years(1)).count() != 12) return false;
  if (duration_cast<days>(weeks(3)).count() != 21) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
