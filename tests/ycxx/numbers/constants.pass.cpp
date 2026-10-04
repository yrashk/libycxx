// [math.constants]/1: the library-defined partial specializations of the mathematical constant
// variable templates (for floating_point T) are initialized with the nearest representable
// values of e, log2(e), log10(e), pi, 1/pi, 1/sqrt(pi), ln 2, ln 10, sqrt(2), sqrt(3),
// 1/sqrt(3), the Euler-Mascheroni constant and the golden ratio. [numbers.syn]: the inline
// constexpr double variables e, log2e, ... equal the _v<double> specializations.
// A decimal literal of 50 significant digits is rounded to the nearest value of its type, so
// it equals the nearest representable value of the constant.
#include <numbers>
#include <type_traits>
#include "check.hpp"

#define CONSTANTS(X)                                                   \
  X(e, 2.7182818284590452353602874713526624977572470937000)            \
  X(log2e, 1.4426950408889634073599246810018921374266459541530)        \
  X(log10e, 0.43429448190325182765112891891660508229439700580366)      \
  X(pi, 3.1415926535897932384626433832795028841971693993751)           \
  X(inv_pi, 0.31830988618379067153776752674502872406891929148091)      \
  X(inv_sqrtpi, 0.56418958354775628694807945156077258584405062932900)  \
  X(ln2, 0.69314718055994530941723212145817656807550013436026)         \
  X(ln10, 2.3025850929940456840179914546843642076011014886288)         \
  X(sqrt2, 1.4142135623730950488016887242096980785696718753769)        \
  X(sqrt3, 1.7320508075688772935274463415058723669428052538104)        \
  X(inv_sqrt3, 0.57735026918962576450914878050195745564760175127013)   \
  X(egamma, 0.57721566490153286060651209008240243104215933593992)      \
  X(phi, 1.6180339887498948482045868343656381177203091798058)

#define CAT2(a, b) a##b
#define CAT(a, b) CAT2(a, b)
#define CHECK_ONE(name, value)                                                         \
  static_assert(std::is_same_v<decltype(std::numbers::name), const double>);           \
  static_assert(std::is_same_v<decltype(std::numbers::name##_v<float>), const float>); \
  static_assert(std::numbers::name == value);                                          \
  static_assert(std::numbers::name == std::numbers::name##_v<double>);                 \
  static_assert(std::numbers::name##_v<float> == CAT(value, f));                       \
  static_assert(std::numbers::name##_v<long double> == CAT(value, L));                 \
  static_assert(std::numbers::name##_v<const double> == value);

CONSTANTS(CHECK_ONE)

int main() {
  // Odr-use: the variables are objects with addresses (inline variables).
  const double* p = &std::numbers::pi;
  CHECK(*p == std::numbers::pi_v<double>);
  CHECK(&std::numbers::e == &std::numbers::e);
  return 0;
}
