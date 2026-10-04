// [time.duration.nonmember]: binary +, - give common_type of the durations (CD(CD(l).count() op
// CD(r).count())); d * s, s * d, d / s, d % s give duration<common_type_t<Rep1, Rep2>, Period>;
// d1 / d2 gives common_type_t<Rep1, Rep2>; d1 % d2 gives the common duration. The scalar forms are
// constrained on convertibility of the scalar to the common rep, and / and % on Rep2 not being a
// specialization of duration.
#include <chrono>
#include <ratio>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using third = duration<int, std::ratio<1, 3>>;
using half = duration<long, std::ratio<1, 2>>;

static_assert(std::is_same_v<decltype(seconds(1) + milliseconds(1)), milliseconds>);
static_assert(std::is_same_v<decltype(minutes(1) - hours(1)), minutes>);
static_assert(std::is_same_v<decltype(third(1) + half(1)), duration<long, std::ratio<1, 6>>>);
static_assert(std::is_same_v<decltype(seconds(1) * 2.5), duration<double>>);
static_assert(std::is_same_v<decltype(2.5 * seconds(1)), duration<double>>);
static_assert(std::is_same_v<decltype(duration<int>(1) * 2LL), duration<long long>>);
static_assert(std::is_same_v<decltype(seconds(1) / 2.0), duration<double>>);
static_assert(std::is_same_v<decltype(duration<int>(1) % 2LL), duration<long long>>);
static_assert(std::is_same_v<decltype(seconds(1) / milliseconds(1)), std::common_type_t<seconds::rep, milliseconds::rep>>);
static_assert(std::is_same_v<decltype(duration<int>(1) / duration<double>(1)), double>);
static_assert(std::is_same_v<decltype(seconds(1) % milliseconds(7)), milliseconds>);

// Constraints.
template <class A, class B> concept can_mul = requires(A a, B b) { a * b; };
template <class A, class B> concept can_div = requires(A a, B b) { a / b; };
template <class A, class B> concept can_mod = requires(A a, B b) { a % b; };
struct not_convertible {};
static_assert(!can_mul<seconds, seconds>);
static_assert(!can_mul<seconds, not_convertible> && !can_mul<not_convertible, seconds>);
static_assert(!can_div<int, seconds>);
static_assert(!can_div<seconds, not_convertible>);
static_assert(can_mod<seconds, int> && can_mod<seconds, milliseconds>);

constexpr bool test() {
  if ((seconds(1) + milliseconds(500)).count() != 1500) return false;
  if ((milliseconds(500) - seconds(1)).count() != -500) return false;
  if ((third(1) + half(1)).count() != 5) return false;  // 2/6 + 3/6
  if ((seconds(3) * 4).count() != 12 || (4 * seconds(3)).count() != 12) return false;
  if ((seconds(7) / 2).count() != 3) return false;
  if ((seconds(7) / 2.0).count() != 3.5) return false;
  if (seconds(3) / milliseconds(1) != 3000) return false;
  if (minutes(1) / seconds(7) != 8) return false;
  if ((seconds(7) % 3).count() != 1) return false;
  if ((seconds(1) % milliseconds(300)).count() != 100) return false;
  if ((minutes(-5) % seconds(70)).count() != -20) return false;  // sign of the dividend
  if ((hours(1) - minutes(1)).count() != 59) return false;
  if (duration<double>(1) / duration<double, std::milli>(250) != 4.0) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
