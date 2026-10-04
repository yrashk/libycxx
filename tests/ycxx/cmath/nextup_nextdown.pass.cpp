// [cmath.syn]: C++26 adds nextup/nextupf/nextupl and nextdown/nextdownf/nextdownl (from C23,
// ISO/IEC 9899:2024 7.12.11.5-6), all constexpr. nextup(x) is the next representable value
// greater than x; nextup(+inf) = +inf, nextup(-inf) = -max, nextup(+-0) = denorm_min, NaN in
// gives NaN; nextdown(x) == -nextup(-x).
#include <cmath>
#include <limits>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::nextup(1.0f)), float>);
static_assert(std::is_same_v<decltype(std::nextdown(1.0)), double>);
static_assert(std::is_same_v<decltype(std::nextupl(1.0L)), long double>);
static_assert(std::is_same_v<decltype(std::nextdownf(1.0f)), float>);
static_assert(std::is_same_v<decltype(std::nextup(1)), double>);

template <class T>
constexpr bool check() {
  using L = std::numeric_limits<T>;
  if (std::nextup(T(1)) != T(1) + L::epsilon()) return false;
  if (std::nextdown(T(1)) != T(1) - L::epsilon() / 2) return false;
  if (std::nextup(T(0)) != L::denorm_min() || std::nextup(-T(0)) != L::denorm_min()) return false;
  if (std::nextdown(T(0)) != -L::denorm_min()) return false;
  if (std::nextup(L::infinity()) != L::infinity() || std::nextup(-L::infinity()) != -L::max()) return false;
  if (std::nextdown(-L::infinity()) != -L::infinity() || std::nextdown(L::infinity()) != L::max()) return false;
  if (std::nextup(L::max()) != L::infinity()) return false;
  if (std::nextup(-L::denorm_min()) != 0 || !std::signbit(std::nextup(-L::denorm_min()))) return false;
  if (!std::isnan(std::nextup(L::quiet_NaN())) || !std::isnan(std::nextdown(L::quiet_NaN()))) return false;
  return true;
}
static_assert(check<float>() && check<double>() && check<long double>());
static_assert(std::nextupf(1.0f) == 1.0f + std::numeric_limits<float>::epsilon());
static_assert(std::nextdownl(1.0L) == 1.0L - std::numeric_limits<long double>::epsilon() / 2);

int main() {
  CHECK(check<float>() && check<double>() && check<long double>());
  volatile double x = 2.0;
  CHECK(std::nextup(x) > 2.0 && std::nextdown(std::nextup(x)) == 2.0);
  return 0;
}
