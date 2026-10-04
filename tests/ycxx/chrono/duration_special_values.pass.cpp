// [time.duration.special]: zero(), min(), max() are noexcept and return
// duration(duration_values<rep>::zero()/min()/max()). [time.traits.duration.values]: zero() is
// Rep(0), min() is numeric_limits<Rep>::lowest(), max() is numeric_limits<Rep>::max(), all
// constexpr and noexcept.
#include <chrono>
#include <limits>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;
using std::numeric_limits;

static_assert(noexcept(seconds::zero()) && noexcept(seconds::min()) && noexcept(seconds::max()));
static_assert(noexcept(duration_values<int>::zero()) && noexcept(duration_values<double>::max()));
static_assert(std::is_same_v<decltype(seconds::zero()), seconds>);
static_assert(std::is_same_v<decltype(duration_values<short>::min()), short>);

static_assert(seconds::zero().count() == 0);
static_assert(seconds::min().count() == numeric_limits<seconds::rep>::lowest());
static_assert(seconds::max().count() == numeric_limits<seconds::rep>::max());
static_assert(duration<double>::min().count() == -numeric_limits<double>::max());  // lowest, not denorm
static_assert(duration<double>::max().count() == numeric_limits<double>::max());
static_assert(duration<float, std::milli>::min().count() == numeric_limits<float>::lowest());
static_assert(duration<unsigned>::min().count() == 0u);
static_assert(duration_values<int>::zero() == 0);
static_assert(duration_values<int>::min() == numeric_limits<int>::min());
static_assert(duration_values<long double>::min() == numeric_limits<long double>::lowest());
static_assert(duration_values<unsigned char>::max() == 255);

int main() {
  CHECK(milliseconds::zero() == seconds::zero());
  CHECK(minutes::min() < minutes::zero() && minutes::zero() < minutes::max());
}
