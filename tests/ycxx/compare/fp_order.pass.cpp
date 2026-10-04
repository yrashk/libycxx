// [cmp.alg]/1.3: strong_order on a floating-point type is consistent with T's comparison
// operators and, when numeric_limits<T>::is_iec559, with ISO/IEC 60559 totalOrder.
// [cmp.alg]/2.3: weak_order on floating point groups values into the equivalence classes
// "all negative NaNs < -inf < negative normals < negative subnormals < both zeros <
// positive subnormals < positive normals < +inf < all positive NaNs".
// [cmp.alg]/3.3: partial_order uses compare_three_way (built-in <=>).
#include <bit>
#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>
#include "check.hpp"

using std::partial_ordering;
using std::strong_ordering;
using std::weak_ordering;

static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<float>::is_iec559);

constexpr double d_pnan = std::bit_cast<double>(std::uint64_t(0x7FF8000000000000));
constexpr double d_pnan2 = std::bit_cast<double>(std::uint64_t(0x7FF8000000000123));
constexpr double d_nnan = std::bit_cast<double>(std::uint64_t(0xFFF8000000000000));
constexpr double d_nnan2 = std::bit_cast<double>(std::uint64_t(0xFFF8000000000456));
constexpr double d_inf = std::numeric_limits<double>::infinity();
constexpr double d_max = std::numeric_limits<double>::max();
constexpr double d_min = std::numeric_limits<double>::min();        // smallest normal
constexpr double d_den = std::numeric_limits<double>::denorm_min(); // smallest subnormal

// Ascending in totalOrder (all distinct representations).
constexpr double ascending[] = {d_nnan, -d_inf, -d_max, -1.0, -d_min, -d_den, -0.0,
                                0.0,    d_den,  d_min,  1.0,  d_max,  d_inf,  d_pnan};

constexpr bool test_strong() {
  static_assert(std::is_same_v<decltype(std::strong_order(1.0, 2.0)), strong_ordering>);
  constexpr int n = sizeof(ascending) / sizeof(ascending[0]);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      strong_ordering o = std::strong_order(ascending[i], ascending[j]);
      if (i < j && o != strong_ordering::less) return false;
      if (i > j && o != strong_ordering::greater) return false;
      if (i == j && o != strong_ordering::equal) return false;
    }
  if (std::strong_order(-0.0, 0.0) != strong_ordering::less) return false;
  if (std::strong_order(0.0, -0.0) != strong_ordering::greater) return false;
  if (std::strong_order(d_pnan, d_pnan) != strong_ordering::equal) return false;
  if (std::strong_order(d_nnan, d_nnan) != strong_ordering::equal) return false;
  if (std::strong_order(d_pnan, d_inf) != strong_ordering::greater) return false;
  if (std::strong_order(d_nnan, -d_inf) != strong_ordering::less) return false;
  if (std::strong_order(d_nnan2, d_pnan2) != strong_ordering::less) return false;
  if (std::strong_order(-1.5f, 2.5f) != strong_ordering::less) return false;
  if (std::strong_order(-0.0f, 0.0f) != strong_ordering::less) return false;
  if (std::strong_order(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()) != strong_ordering::greater)
    return false;
  return true;
}

constexpr bool test_weak() {
  static_assert(std::is_same_v<decltype(std::weak_order(1.0, 2.0)), weak_ordering>);
  if (std::weak_order(-0.0, 0.0) != weak_ordering::equivalent) return false;
  if (std::weak_order(0.0, -0.0) != weak_ordering::equivalent) return false;
  if (std::weak_order(d_pnan, d_pnan2) != weak_ordering::equivalent) return false;   // all +NaNs together
  if (std::weak_order(d_nnan, d_nnan2) != weak_ordering::equivalent) return false;   // all -NaNs together
  if (std::weak_order(d_nnan, d_pnan) != weak_ordering::less) return false;
  if (std::weak_order(d_nnan, -d_inf) != weak_ordering::less) return false;
  if (std::weak_order(-d_inf, -d_max) != weak_ordering::less) return false;
  if (std::weak_order(-d_min, -d_den) != weak_ordering::less) return false;
  if (std::weak_order(-d_den, -0.0) != weak_ordering::less) return false;
  if (std::weak_order(0.0, d_den) != weak_ordering::less) return false;
  if (std::weak_order(d_den, d_min) != weak_ordering::less) return false;
  if (std::weak_order(d_max, d_inf) != weak_ordering::less) return false;
  if (std::weak_order(d_inf, d_pnan) != weak_ordering::less) return false;
  if (std::weak_order(d_pnan, d_inf) != weak_ordering::greater) return false;
  if (std::weak_order(1.0, 1.0) != weak_ordering::equivalent) return false;
  if (std::weak_order(2.0, 1.0) != weak_ordering::greater) return false;
  if (std::weak_order(std::numeric_limits<float>::quiet_NaN(), 0.0f) != weak_ordering::greater) return false;
  if (std::weak_order(-0.0f, 0.0f) != weak_ordering::equivalent) return false;
  return true;
}

constexpr bool test_partial() {
  static_assert(std::is_same_v<decltype(std::partial_order(1.0, 2.0)), partial_ordering>);
  if (std::partial_order(-0.0, 0.0) != partial_ordering::equivalent) return false;
  if (std::partial_order(d_pnan, d_pnan) != partial_ordering::unordered) return false;
  if (std::partial_order(d_nnan, 0.0) != partial_ordering::unordered) return false;
  if (std::partial_order(1.0, d_pnan) != partial_ordering::unordered) return false;
  if (std::partial_order(-d_inf, d_inf) != partial_ordering::less) return false;
  if (std::partial_order(2.0, 1.0) != partial_ordering::greater) return false;
  if (std::partial_order(1.0f, 1.0f) != partial_ordering::equivalent) return false;
  return true;
}

static_assert(test_strong());
static_assert(test_weak());
static_assert(test_partial());

int main() {
  CHECK(test_strong());
  CHECK(test_weak());
  CHECK(test_partial());
  // long double: is_iec559 drives the same requirements.
  if constexpr (std::numeric_limits<long double>::is_iec559) {
    volatile long double z = 0.0L;
    long double pz = z, nz = -pz;
    long double inf = std::numeric_limits<long double>::infinity();
    long double nan = std::numeric_limits<long double>::quiet_NaN();
    CHECK(std::strong_order(nz, pz) == strong_ordering::less);
    CHECK(std::strong_order(nan, inf) == strong_ordering::greater);
    CHECK(std::strong_order(-nan, -inf) == strong_ordering::less);
    CHECK(std::weak_order(nz, pz) == weak_ordering::equivalent);
    CHECK(std::weak_order(nan, inf) == weak_ordering::greater);
    CHECK(std::partial_order(nan, pz) == partial_ordering::unordered);
  }
  // Run-time values.
  volatile double v = 0.0;
  double pz = v, nz = -pz;
  CHECK(std::strong_order(nz, pz) == strong_ordering::less);
  CHECK(std::weak_order(nz, pz) == weak_ordering::equivalent);
  CHECK(std::partial_order(nz, pz) == partial_ordering::equivalent);
  return 0;
}
