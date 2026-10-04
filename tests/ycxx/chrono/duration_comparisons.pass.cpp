// [time.duration.comparisons]: ==, <, >, <=, >= and <=> compare CT(lhs).count() with
// CT(rhs).count(), CT the common type; <=> requires three_way_comparable<CT::rep> and returns the
// result of <=> on the counts.
#include <chrono>
#include <compare>
#include <ratio>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

static_assert(std::is_same_v<decltype(seconds(1) <=> milliseconds(1)), std::strong_ordering>);
static_assert(std::is_same_v<decltype(duration<double>(1) <=> seconds(1)), std::partial_ordering>);
static_assert(std::is_same_v<decltype(seconds(1) == milliseconds(1)), bool>);

constexpr bool test() {
  if (!(seconds(1) == milliseconds(1000)) || seconds(1) != milliseconds(1000)) return false;
  if (seconds(1) == milliseconds(1001)) return false;
  if (!(seconds(1) < milliseconds(1001)) || seconds(1) < milliseconds(1000)) return false;
  if (!(minutes(2) > seconds(119)) || !(minutes(2) >= seconds(120)) || !(minutes(2) <= seconds(120))) return false;
  if ((minutes(1) <=> seconds(61)) != std::strong_ordering::less) return false;
  if ((hours(1) <=> minutes(60)) != std::strong_ordering::equal) return false;
  if ((duration<int, std::ratio<1, 3>>(1) <=> duration<int, std::ratio<1, 2>>(1)) != std::strong_ordering::less)
    return false;
  if (!(duration<double>(0.5) == milliseconds(500))) return false;
  if (!(seconds(-1) < seconds(0))) return false;
  duration<double> nan(__builtin_nan(""));
  if ((nan <=> duration<double>(0)) != std::partial_ordering::unordered) return false;
  if (nan == nan) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
