// [c.math.lerp]: constexpr floating-point-type lerp(a, b, t) noexcept. Returns a + t(b - a).
// For finite a and b: t == 0 gives exactly a, t == 1 gives exactly b, t in [0, 1] gives a
// finite result, a == b and finite t gives a, and for finite t (or non-NaN t with b - a != 0)
// the result is not a NaN; the result is monotonic in t (the product of CMP(lerp(t2),
// lerp(t1)), CMP(t2, t1) and CMP(b, a) is non-negative). Integer arguments are treated as
// double ([cmath.syn]/3).
#include <cmath>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include "check.hpp"

static_assert(noexcept(std::lerp(1.0, 2.0, 0.5)));
static_assert(std::is_same_v<decltype(std::lerp(1.0f, 2.0f, 0.5f)), float>);
static_assert(std::is_same_v<decltype(std::lerp(1.0L, 2.0L, 0.5L)), long double>);
static_assert(std::is_same_v<decltype(std::lerp(1, 2, 0.5f)), double>);
static_assert(std::is_same_v<decltype(std::lerp(1.0f, 2.0, 0.5f)), double>);
static_assert(std::lerp(0.0, 10.0, 0.5) == 5.0);
static_assert(std::lerp(2.0f, 4.0f, 0.25f) == 2.5f);
static_assert(std::lerp(1, 3, 2) == 5.0);  // extrapolation

int cmp(double x, double y) { return x > y ? 1 : x < y ? -1 : 0; }

template <class T>
void exactness() {
  const T vals[] = {T(-1e30), T(-3.7), T(-0.1), T(0), T(0.1), T(1), T(2.5), T(1e20)};
  for (T a : vals)
    for (T b : vals) {
      CHECK(std::lerp(a, b, T(0)) == a);
      CHECK(std::lerp(a, b, T(1)) == b);
      for (T t : {T(0), T(0.125), T(0.3), T(0.5), T(0.999), T(1)}) CHECK(std::isfinite(std::lerp(a, b, t)));
      CHECK(std::lerp(a, a, T(0.3)) == a && std::lerp(a, a, T(17)) == a && std::lerp(a, a, T(-4)) == a);
      CHECK(!std::isnan(std::lerp(a, b, T(5))));
      if (b != a) CHECK(!std::isnan(std::lerp(a, b, std::numeric_limits<T>::infinity())));
    }
}

int main() {
  exactness<float>();
  exactness<double>();
  exactness<long double>();
  // Monotonicity in t, including values just around 1.
  const double ab[][2] = {{0.1, 0.7}, {-3.0, 1e-3}, {5.0, -2.0}, {1e16, 1e16 + 2}, {0.3, 0.30000000000000004}};
  for (auto& p : ab) {
    double prev_t = -0.5, prev = std::lerp(p[0], p[1], prev_t);
    for (double t = -0.5; t <= 1.5; t += 1.0 / 64) {
      for (double tt : {std::nextafter(t, -1.0), t, std::nextafter(t, 2.0)}) {
        double r = std::lerp(p[0], p[1], tt);
        CHECK(cmp(r, prev) * cmp(tt, prev_t) * cmp(p[1], p[0]) >= 0);
        prev = r;
        prev_t = tt;
      }
    }
  }
  CHECK(std::lerp(1.0, 2.0, 0.5) == 1.5);
  CHECK(std::lerp(-1.0, 1.0, 0.75) == 0.5);
  return 0;
}
