// [time.duration.arithmetic]: unary + and - return common_type_t<duration>; ++, --, +=, -=, *=,
// /=, %= (by rep and by duration) act on rep_ and return *this (or the old value for postfix).
#include <chrono>
#include <ratio>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

using d24 = duration<int, std::ratio<2, 4>>;  // period ratio<1, 2>
static_assert(std::is_same_v<decltype(+seconds(1)), seconds>);
static_assert(std::is_same_v<decltype(-seconds(1)), seconds>);
static_assert(std::is_same_v<decltype(+d24(1)), std::common_type_t<d24>>);
static_assert(std::is_same_v<decltype(-d24(1)), duration<int, std::ratio<1, 2>>>);
static_assert(std::is_same_v<decltype(++std::declval<seconds&>()), seconds&>);
static_assert(std::is_same_v<decltype(std::declval<seconds&>()++), seconds>);
static_assert(std::is_same_v<decltype(std::declval<seconds&>() += seconds(1)), seconds&>);
static_assert(std::is_same_v<decltype(std::declval<seconds&>() %= seconds(1)), seconds&>);
static_assert(std::is_same_v<decltype(std::declval<seconds&>() *= 2), seconds&>);

constexpr bool test() {
  seconds s(10);
  if ((+s).count() != 10 || (-s).count() != -10) return false;
  if ((++s).count() != 11) return false;
  if ((s++).count() != 11 || s.count() != 12) return false;
  if ((--s).count() != 11) return false;
  if ((s--).count() != 11 || s.count() != 10) return false;
  s += seconds(5);
  if (s.count() != 15) return false;
  s -= seconds(20);
  if (s.count() != -5) return false;
  s *= -3;
  if (s.count() != 15) return false;
  s /= 4;
  if (s.count() != 3) return false;  // integer division
  s = seconds(17);
  s %= 5;
  if (s.count() != 2) return false;
  s = seconds(17);
  s %= seconds(6);
  if (s.count() != 5) return false;
  seconds& r = (s += seconds(1));
  if (&r != &s) return false;
  duration<double> d(1.5);
  d *= 2.0;
  d /= 4.0;
  if (d.count() != 0.75) return false;
  // Unsigned rep: unary minus is computed in the rep.
  duration<unsigned> u(1);
  if ((-u).count() != unsigned(-1)) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
