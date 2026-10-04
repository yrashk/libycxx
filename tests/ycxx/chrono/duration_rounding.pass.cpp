// [time.duration.cast]/4-9: floor<To>(d) is the greatest t representable in To with t <= d;
// ceil<To>(d) the least with t >= d; round<To>(d) the closest value, ties to the even one
// (t % 2 == 0), and requires treat_as_floating_point_v<To::rep> to be false. All are constrained on
// To being a specialization of duration.
#include <chrono>
#include <ratio>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

template <class To, class From> concept can_floor = requires(From f) { floor<To>(f); };
template <class To, class From> concept can_ceil = requires(From f) { ceil<To>(f); };
template <class To, class From> concept can_round = requires(From f) { round<To>(f); };
static_assert(can_floor<seconds, milliseconds> && can_ceil<seconds, milliseconds> && can_round<seconds, milliseconds>);
static_assert(can_floor<duration<double>, milliseconds> && can_ceil<duration<double>, milliseconds>);
static_assert(!can_round<duration<double>, milliseconds>);
static_assert(!can_floor<int, seconds> && !can_ceil<long, seconds> && !can_round<int, seconds>);
static_assert(std::is_same_v<decltype(floor<minutes>(seconds(1))), minutes>);

constexpr bool test() {
  struct row { long long ms, fl, ce, ro; };
  constexpr row rows[] = {
      {0, 0, 0, 0},          {1, 0, 1, 0},          {499, 0, 1, 0},       {500, 0, 1, 0},
      {501, 0, 1, 1},        {1000, 1, 1, 1},       {1499, 1, 2, 1},      {1500, 1, 2, 2},
      {2500, 2, 3, 2},       {3500, 3, 4, 4},       {-1, -1, 0, 0},       {-500, -1, 0, 0},
      {-501, -1, 0, -1},     {-1500, -2, -1, -2},   {-2500, -3, -2, -2},  {-3500, -4, -3, -4},
      {-1999, -2, -1, -2},   {-2000, -2, -2, -2},
  };
  for (const row& r : rows) {
    milliseconds d(r.ms);
    if (floor<seconds>(d).count() != r.fl) return false;
    if (ceil<seconds>(d).count() != r.ce) return false;
    if (round<seconds>(d).count() != r.ro) return false;
  }
  // From floating point.
  if (floor<seconds>(duration<double>(1.7)).count() != 1) return false;
  if (floor<seconds>(duration<double>(-1.2)).count() != -2) return false;
  if (ceil<seconds>(duration<double>(1.2)).count() != 2) return false;
  if (ceil<seconds>(duration<double>(-1.7)).count() != -1) return false;
  if (round<seconds>(duration<double>(2.5)).count() != 2) return false;
  if (round<seconds>(duration<double>(-0.5)).count() != 0) return false;
  if (round<seconds>(duration<double>(0.75)).count() != 1) return false;
  // To a floating-point duration: exactly representable values are returned unchanged.
  if (floor<duration<double>>(milliseconds(1500)).count() != 1.5) return false;
  if (ceil<duration<double>>(milliseconds(-250)).count() != -0.25) return false;
  // Coarser periods.
  if (floor<minutes>(seconds(-1)).count() != -1) return false;
  if (ceil<minutes>(seconds(60)).count() != 1 || ceil<minutes>(seconds(61)).count() != 2) return false;
  if (round<hours>(minutes(90)).count() != 2 || round<hours>(minutes(150)).count() != 2) return false;
  if (floor<days>(hours(-1)).count() != -1) return false;
  if (round<duration<int, std::ratio<1, 2>>>(milliseconds(250)).count() != 0) return false;  // 0.5 halves: tie to 0
  if (round<duration<int, std::ratio<1, 2>>>(milliseconds(750)).count() != 2) return false;  // 1.5: tie to 2
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
