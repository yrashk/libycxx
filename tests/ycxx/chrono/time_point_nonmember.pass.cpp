// [time.point.nonmember]: tp + d and d + tp give time_point<Clock, common_type_t<Duration1,
// duration<Rep2, Period2>>>; tp - d likewise; tp1 - tp2 gives common_type_t<Duration1, Duration2>.
// [time.point.comparisons]: comparisons of time_since_epoch(); <=> requires
// three_way_comparable_with of the durations.
#include <chrono>
#include <compare>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using tp_s = time_point<steady_clock, seconds>;
using tp_ms = time_point<steady_clock, milliseconds>;
using tp_min = time_point<steady_clock, minutes>;

static_assert(std::is_same_v<decltype(tp_s() + milliseconds(1)), tp_ms>);
static_assert(std::is_same_v<decltype(milliseconds(1) + tp_s()), tp_ms>);
static_assert(std::is_same_v<decltype(tp_min() - seconds(1)), tp_s>);
static_assert(std::is_same_v<decltype(tp_min() - tp_ms()), milliseconds>);
static_assert(std::is_same_v<decltype(tp_s() + duration<double>(1))::duration, duration<double>>);
static_assert(std::is_same_v<decltype(tp_s() <=> tp_ms()), std::strong_ordering>);

template <class A, class B> concept can_sub = requires(A a, B b) { a - b; };
template <class A, class B> concept can_add = requires(A a, B b) { a + b; };
static_assert(!can_add<tp_s, tp_s>);
static_assert(!can_sub<seconds, tp_s>);
static_assert(!can_sub<tp_s, time_point<system_clock, seconds>>);

constexpr bool test() {
  tp_s a(seconds(100));
  tp_ms b = a + milliseconds(250);
  if (b.time_since_epoch().count() != 100250) return false;
  if ((milliseconds(250) + a) != b) return false;
  if ((b - milliseconds(250)) != a) return false;
  if ((b - a).count() != 250) return false;
  if ((a - b).count() != -250) return false;
  tp_min c(minutes(2));
  if ((c - a).count() != 20) return false;
  if (!(a < b) || !(b > a) || !(a <= a) || !(a >= a) || !(a != b)) return false;
  if (!(tp_ms(milliseconds(100000)) == a)) return false;
  if ((a <=> b) != std::strong_ordering::less) return false;
  if ((c <=> tp_s(seconds(120))) != std::strong_ordering::equal) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
