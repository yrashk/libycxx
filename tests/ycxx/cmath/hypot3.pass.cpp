// [c.math.hypot3]: constexpr floating-point-type hypot(x, y, z); Returns: sqrt(x^2 + y^2 + z^2).
// [cmath.syn]/3: with mixed arithmetic arguments every argument is cast to the floating-point
// type of greatest rank, integers counting as double. Intermediate overflow must not spoil a
// representable result: the mathematical value is returned, not an overflowed infinity.
// COUNTERPART: libcxx:numerics/c.math/cmath.pass.cpp
#include <cmath>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::hypot(1.0f, 2.0f, 2.0f)), float>);
static_assert(std::is_same_v<decltype(std::hypot(1.0, 2.0, 2.0)), double>);
static_assert(std::is_same_v<decltype(std::hypot(1.0L, 2.0L, 2.0L)), long double>);
static_assert(std::is_same_v<decltype(std::hypot(1, 2, 2)), double>);
static_assert(std::is_same_v<decltype(std::hypot(1.0f, 2, 2.0f)), double>);
static_assert(std::is_same_v<decltype(std::hypot(1.0f, 2.0L, 2)), long double>);

static_assert(std::hypot(2.0, 3.0, 6.0) == 7.0);  // constexpr since C++26

bool close(long double a, long double b, long double tol) { return std::fabs(a - b) <= tol * std::fabs(b); }

int main() {
  CHECK(std::hypot(1.0f, 2.0f, 2.0f) == 3.0f);
  CHECK(std::hypot(2.0, 3.0, 6.0) == 7.0);
  CHECK(std::hypot(-2.0L, 3.0L, -6.0L) == 7.0L);
  CHECK(std::hypot(1, 4, 8) == 9.0);
  CHECK(std::hypot(0.0, 0.0, 0.0) == 0.0);
  CHECK(close(std::hypot(1.0, 1.0, 1.0), 1.7320508075688772935L, 1e-15L));
  // Large and small components: no spurious overflow or underflow.
  const double big = std::numeric_limits<double>::max() / 2;
  CHECK(close(std::hypot(big, big, 0.0), big * 1.4142135623730950488L, 1e-15L));
  CHECK(std::isfinite(std::hypot(big, big, 0.0)));
  const float bigf = std::numeric_limits<float>::max() / 2;
  CHECK(std::isfinite(std::hypot(bigf, bigf, bigf)));
  return 0;
}
