// [c.math.fpclass]/1: "The classification / comparison functions behave the same as the C
// macros with the corresponding names defined in the C standard library." [cmath.syn]/2-3:
// an overload for each cv-unqualified floating-point type, and arguments of integer type are
// effectively cast to double. fpclassify returns int, the others bool.
#include <cmath>
#include <limits>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::fpclassify(1.0f)), int>);
static_assert(std::is_same_v<decltype(std::isnan(1.0)), bool>);
static_assert(std::is_same_v<decltype(std::isfinite(1.0L)), bool>);
static_assert(std::is_same_v<decltype(std::signbit(1)), bool>);
static_assert(std::is_same_v<decltype(std::isless(1, 2.0f)), bool>);

template <class T>
void check_type() {
  using L = std::numeric_limits<T>;
  const T inf = L::infinity(), nan = L::quiet_NaN(), den = L::denorm_min(), mn = L::min();
  CHECK(std::fpclassify(T(0)) == FP_ZERO);
  CHECK(std::fpclassify(-T(0)) == FP_ZERO);
  CHECK(std::fpclassify(T(1)) == FP_NORMAL);
  CHECK(std::fpclassify(mn) == FP_NORMAL);
  CHECK(std::fpclassify(den) == FP_SUBNORMAL);
  CHECK(std::fpclassify(inf) == FP_INFINITE);
  CHECK(std::fpclassify(-inf) == FP_INFINITE);
  CHECK(std::fpclassify(nan) == FP_NAN);

  CHECK(std::isfinite(T(1)) && std::isfinite(den) && !std::isfinite(inf) && !std::isfinite(nan));
  CHECK(std::isinf(inf) && std::isinf(-inf) && !std::isinf(L::max()) && !std::isinf(nan));
  CHECK(std::isnan(nan) && std::isnan(-nan) && !std::isnan(inf) && !std::isnan(T(0)));
  CHECK(std::isnormal(T(-2)) && std::isnormal(mn) && !std::isnormal(den) && !std::isnormal(T(0)) &&
        !std::isnormal(inf) && !std::isnormal(nan));
  CHECK(!std::signbit(T(0)) && std::signbit(-T(0)) && std::signbit(-inf) && !std::signbit(inf) &&
        std::signbit(T(-1)) && std::signbit(-den));
  CHECK(std::signbit(std::copysign(nan, T(-1))) && !std::signbit(std::copysign(nan, T(1))));

  // Quiet comparisons: false (not an exception) for unordered operands.
  CHECK(std::isgreater(T(2), T(1)) && !std::isgreater(T(1), T(1)) && !std::isgreater(nan, T(1)));
  CHECK(std::isgreaterequal(T(1), T(1)) && !std::isgreaterequal(nan, nan));
  CHECK(std::isless(-inf, T(0)) && !std::isless(T(0), -T(0)) && !std::isless(T(1), nan));
  CHECK(std::islessequal(T(0), -T(0)) && !std::islessequal(nan, inf));
  CHECK(std::islessgreater(T(1), T(2)) && std::islessgreater(T(2), T(1)) &&
        !std::islessgreater(T(1), T(1)) && !std::islessgreater(nan, T(1)));
  CHECK(std::isunordered(nan, T(1)) && std::isunordered(T(1), nan) && !std::isunordered(inf, -inf));
}

int main() {
  check_type<float>();
  check_type<double>();
  check_type<long double>();

  // Integer arguments are treated as double.
  CHECK(std::fpclassify(0) == FP_ZERO);
  CHECK(std::fpclassify(7L) == FP_NORMAL);
  CHECK(std::isfinite(3) && !std::isinf(3) && !std::isnan(3u) && std::isnormal(5LL) && !std::isnormal(0));
  CHECK(std::signbit(-4) && !std::signbit(4u));
  // Mixed arguments: compared after conversion to the common floating-point type.
  CHECK(std::isless(1, 1.5) && std::isgreater(2.5f, 2) && std::islessequal(3, 3.0L));
  CHECK(std::isunordered(std::numeric_limits<float>::quiet_NaN(), 1));
  // 2^24 + 1 is not representable in float but both operands become double here.
  CHECK(std::isless(16777216.0f, 16777217));
  return 0;
}
