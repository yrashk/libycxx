// [time.point]: member types clock, duration, rep, period; time_point() is the epoch;
// explicit time_point(d) is epoch + d; time_point(const time_point<clock, Duration2>&) is
// constrained on is_convertible_v<Duration2, duration>; time_since_epoch(); ++, --, +=, -=;
// min() and max() are time_point(duration::min()) / time_point(duration::max()), noexcept.
#include <chrono>
#include <ratio>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using tp_s = time_point<system_clock, seconds>;
using tp_ms = time_point<system_clock, milliseconds>;

static_assert(std::is_same_v<time_point<system_clock>::duration, system_clock::duration>);
static_assert(std::is_same_v<tp_s::clock, system_clock>);
static_assert(std::is_same_v<tp_s::duration, seconds>);
static_assert(std::is_same_v<tp_s::rep, seconds::rep>);
static_assert(std::is_same_v<tp_s::period, std::ratio<1>>);
static_assert(std::is_same_v<time_point<system_clock, duration<int, std::ratio<2, 4>>>::period, std::ratio<1, 2>>);

static_assert(!std::is_convertible_v<seconds, tp_s>);  // explicit
static_assert(std::is_constructible_v<tp_s, seconds>);
static_assert(std::is_convertible_v<tp_s, tp_ms>);
static_assert(!std::is_convertible_v<tp_ms, tp_s> && !std::is_constructible_v<tp_s, tp_ms>);
static_assert(!std::is_constructible_v<tp_s, time_point<steady_clock, seconds>>);  // other clock
static_assert(std::is_convertible_v<tp_ms, time_point<system_clock, duration<double>>>);
static_assert(noexcept(tp_s::min()) && noexcept(tp_s::max()));
static_assert(std::is_trivially_copyable_v<tp_s>);

constexpr bool test() {
  tp_s epoch;
  if (epoch.time_since_epoch() != seconds(0)) return false;
  tp_s t(seconds(10));
  if (t.time_since_epoch().count() != 10) return false;
  tp_ms m = t;
  if (m.time_since_epoch().count() != 10000) return false;
  if ((++t).time_since_epoch().count() != 11) return false;
  if ((t++).time_since_epoch().count() != 11 || t.time_since_epoch().count() != 12) return false;
  if ((--t).time_since_epoch().count() != 11) return false;
  if ((t--).time_since_epoch().count() != 11 || t.time_since_epoch().count() != 10) return false;
  t += seconds(5);
  if (t.time_since_epoch().count() != 15) return false;
  t -= seconds(20);
  if (t.time_since_epoch().count() != -5) return false;
  tp_s& r = (t += seconds(1));
  if (&r != &t) return false;
  if (tp_s::min().time_since_epoch() != seconds::min()) return false;
  if (tp_s::max().time_since_epoch() != seconds::max()) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
