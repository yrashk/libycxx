// [time.duration.alg]: abs(d): "Constraints: numeric_limits<Rep>::is_signed is true. Returns: If
// d >= d.zero(), return d, otherwise return -d."
#include <chrono>
#include <type_traits>
#include "check.hpp"

using namespace std::chrono;

template <class D> concept has_abs = requires(D d) { std::chrono::abs(d); };
static_assert(has_abs<seconds> && has_abs<duration<double>>);
static_assert(!has_abs<duration<unsigned>> && !has_abs<duration<unsigned long long, std::milli>>);
static_assert(std::is_same_v<decltype(std::chrono::abs(minutes(1))), minutes>);

constexpr bool test() {
  if (std::chrono::abs(seconds(-3)) != seconds(3)) return false;
  if (std::chrono::abs(seconds(3)) != seconds(3)) return false;
  if (std::chrono::abs(seconds(0)) != seconds(0)) return false;
  if (std::chrono::abs(duration<double>(-1.5)).count() != 1.5) return false;
  if (std::chrono::abs(minutes::min() + minutes(1)) != minutes::max()) return false;
  return true;
}
static_assert(test());

int main() { CHECK(test()); }
