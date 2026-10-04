// [cmath.syn]: in C++26 (P0533R9, P1383R2) fabs, ceil, floor, trunc, round, lround, llround,
// fmod, remainder, remquo, copysign, frexp, ldexp, modf, ilogb, logb, scalbn, scalbln,
// nextafter, nexttoward, fdim, fmax, fmin, fma, sqrt, hypot, cbrt and abs are constexpr.
// [library.c]/3: evaluated as a core constant expression they have the semantics of
// ISO/IEC 9899:2024 Annex F; every call below has an exact result and raises no floating-point
// exception other than FE_INEXACT, so it is a constant expression. (nextafter towards a
// subnormal result raises FE_UNDERFLOW, F.10.8.3, so it is not used that way here.)
#include <cmath>
#include <limits>
#include <type_traits>
#include "check.hpp"

template <class T>
constexpr bool exact() {
  using L = std::numeric_limits<T>;
  const T inf = L::infinity();
  if (std::fabs(T(-2.5)) != T(2.5) || std::signbit(std::fabs(-T(0)))) return false;
  if (std::abs(T(-3)) != T(3) || std::fabs(-inf) != inf) return false;
  if (std::ceil(T(1.25)) != T(2) || std::ceil(T(-1.75)) != T(-1)) return false;
  if (!std::signbit(std::ceil(T(-0.5)))) return false;          // ceil(-0.5) == -0
  if (std::floor(T(-1.25)) != T(-2) || std::floor(T(2.75)) != T(2)) return false;
  if (std::trunc(T(-2.75)) != T(-2) || std::trunc(T(2.75)) != T(2)) return false;
  if (std::round(T(2.5)) != T(3) || std::round(T(-2.5)) != T(-3) || std::round(T(0.25)) != T(0)) return false;
  if (std::lround(T(-3.5)) != -4L || std::llround(T(4.5)) != 5LL) return false;
  if (std::fmod(T(7.5), T(2)) != T(1.5) || std::fmod(T(-7.5), T(2)) != T(-1.5)) return false;
  if (std::remainder(T(7.5), T(2)) != T(-0.5) || std::remainder(T(5), T(2)) != T(1)) return false;
  int q = 0;
  if (std::remquo(T(7), T(2), &q) != T(-1) || (q & 7) != 4) return false;  // 7/2 = 3.5 -> 4
  if (std::copysign(T(3), T(-0.0)) != T(-3) || std::copysign(T(-3), T(1)) != T(3)) return false;
  int e = 0;
  if (std::frexp(T(48), &e) != T(0.75) || e != 6) return false;
  if (std::frexp(T(0), &e) != T(0) || e != 0) return false;
  if (std::ldexp(T(0.75), 4) != T(12) || std::scalbn(T(3), -1) != T(1.5) || std::scalbln(T(1), 10L) != T(1024))
    return false;
  T ip = 0;
  if (std::modf(T(-3.25), &ip) != T(-0.25) || ip != T(-3)) return false;
  if (std::ilogb(T(1024)) != 10 || std::ilogb(T(0.375)) != -2) return false;
  if (std::logb(T(1024)) != T(10) || std::logb(T(-0.375)) != T(-2)) return false;
  if (std::nextafter(T(1), T(2)) != T(1) + L::epsilon()) return false;
  if (std::nexttoward(T(1), 0.0L) != T(1) - L::epsilon() / 2) return false;
  if (std::fdim(T(5), T(3)) != T(2) || std::fdim(T(3), T(5)) != T(0)) return false;
  if (std::fmax(T(1), L::quiet_NaN()) != T(1) || std::fmin(L::quiet_NaN(), T(-1)) != T(-1)) return false;
  if (std::fmax(T(-2), T(3)) != T(3) || std::fmin(T(-2), T(3)) != T(-2)) return false;
  if (std::fma(T(2), T(3), T(4)) != T(10)) return false;
  if (std::sqrt(T(6.25)) != T(2.5) || std::sqrt(inf) != inf || std::signbit(std::sqrt(-T(0))) != true) return false;
  if (std::hypot(T(3), T(4)) != T(5) || std::hypot(-inf, L::quiet_NaN()) != inf) return false;
  if (std::cbrt(T(-27)) != T(-3)) return false;
  return true;
}

static_assert(exact<float>());
static_assert(exact<double>());
static_assert(exact<long double>());

// The f/l-suffixed names are constexpr too.
static_assert(std::fabsf(-1.5f) == 1.5f && std::fabsl(-1.5L) == 1.5L);
static_assert(std::floorf(1.5f) == 1.0f && std::ceill(1.5L) == 2.0L);
static_assert(std::fmodf(5.0f, 3.0f) == 2.0f && std::sqrtl(16.0L) == 4.0L);
// Integer arguments are treated as double.
static_assert(std::sqrt(16) == 4.0 && std::is_same_v<decltype(std::sqrt(16)), double>);
static_assert(std::fmax(1, 2.0f) == 2.0 && std::is_same_v<decltype(std::fmax(1, 2.0f)), double>);
// abs on integers ([c.math.abs]) is constexpr.
static_assert(std::abs(-5) == 5 && std::abs(-5L) == 5L && std::abs(-5LL) == 5LL);

int main() {
  CHECK(exact<float>() && exact<double>() && exact<long double>());
  return 0;
}
