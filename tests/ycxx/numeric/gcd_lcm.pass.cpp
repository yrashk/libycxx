// [numeric.ops.gcd]: gcd(m, n) returns zero when both are zero, otherwise the greatest
// common divisor of |m| and |n|; the return type is common_type_t<M, N>. [numeric.ops.lcm]:
// lcm(m, n) returns zero when either is zero, otherwise the least common multiple of |m|
// and |n|. Both are constexpr and accept mixed integer types (including char types and
// unsigned types), but not bool.
#include <numeric>
#include <climits>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::gcd(1, 2L)), long>);
static_assert(std::is_same_v<decltype(std::gcd(short(1), short(2))), short>);  // common_type_t<short, short>
static_assert(std::is_same_v<decltype(std::lcm(1u, 2)), unsigned>);
static_assert(std::is_same_v<decltype(std::gcd((unsigned char)4, (signed char)6)), int>);  // promoted by ?:

constexpr bool test() {
  if (std::gcd(12, 18) != 6 || std::gcd(18, 12) != 6) return false;
  if (std::gcd(0, 0) != 0 || std::gcd(0, 7) != 7 || std::gcd(7, 0) != 7) return false;
  if (std::gcd(-12, 18) != 6 || std::gcd(12, -18) != 6 || std::gcd(-12, -18) != 6) return false;
  if (std::gcd(-7, 0) != 7) return false;
  if (std::gcd(17, 5) != 1) return false;
  if (std::gcd(1LL << 40, 1LL << 35) != (1LL << 35)) return false;
  if (std::gcd(48u, 180L) != 12) return false;
  if (std::gcd('0', 'H') != 24) return false;  // char types are integer types other than bool
  if (std::gcd(INT_MIN, 6LL) != 2) return false;  // |INT_MIN| representable in long long

  if (std::lcm(4, 6) != 12 || std::lcm(6, 4) != 12) return false;
  if (std::lcm(0, 5) != 0 || std::lcm(5, 0) != 0 || std::lcm(0, 0) != 0) return false;
  if (std::lcm(-4, 6) != 12 || std::lcm(4, -6) != 12 || std::lcm(-4, -6) != 12) return false;
  if (std::lcm(7, 7) != 7 || std::lcm(1, 9) != 9) return false;
  if (std::lcm(1u << 20, 3ULL) != 3ULL << 20) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  volatile int m = 21, n = 35;
  CHECK(std::gcd(m, n) == 7 && std::lcm(m, n) == 105);
  return 0;
}
