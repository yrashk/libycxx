// [time.point.cast]: time_point_cast<ToDuration>(t) is time_point<Clock, ToDuration>
// (duration_cast<ToDuration>(t.time_since_epoch())); floor, ceil and round on time_points apply
// the duration versions to time_since_epoch(); all are constrained on ToDuration being a duration,
// round also on its rep not being treated as floating point.
#include <chrono>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using tp_ms = sys_time<milliseconds>;

template <class To, class T> concept can_cast = requires(T t) { time_point_cast<To>(t); };
template <class To, class T> concept can_round = requires(T t) { round<To>(t); };
template <class To, class T> concept can_floor = requires(T t) { floor<To>(t); };
static_assert(can_cast<seconds, tp_ms> && !can_cast<sys_seconds, tp_ms> && !can_cast<int, tp_ms>);
static_assert(can_round<seconds, tp_ms> && !can_round<duration<double>, tp_ms>);
static_assert(can_floor<duration<double>, tp_ms> && !can_floor<sys_seconds, tp_ms>);
static_assert(std::is_same_v<decltype(time_point_cast<seconds>(tp_ms())), sys_time<seconds>>);
static_assert(std::is_same_v<decltype(floor<days>(tp_ms())), sys_days>);
static_assert(std::is_same_v<decltype(ceil<minutes>(time_point<steady_clock, seconds>())),
                             time_point<steady_clock, minutes>>);

constexpr bool test() {
  tp_ms t(milliseconds(-1500));
  if (time_point_cast<seconds>(t).time_since_epoch().count() != -1) return false;
  if (floor<seconds>(t).time_since_epoch().count() != -2) return false;
  if (ceil<seconds>(t).time_since_epoch().count() != -1) return false;
  if (round<seconds>(t).time_since_epoch().count() != -2) return false;
  tp_ms u(milliseconds(2500));
  if (round<seconds>(u).time_since_epoch().count() != 2) return false;
  if (time_point_cast<duration<double>>(u).time_since_epoch().count() != 2.5) return false;
  if (floor<days>(sys_seconds(seconds(-1))).time_since_epoch().count() != -1) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
