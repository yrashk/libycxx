// [cmath.syn]: besides the overloads for each floating-point type, <cmath> declares in namespace
// std the C library's functions with the suffixes f and l, which are not overloaded: Xf takes
// and returns float, Xl long double, and each gives the result of the float / long double
// overload of X (Xf(0.1) converts the double argument to float first). [sf.cmath]: the same
// for the mathematical special functions (assoc_laguerref, ..., sph_neumannl).
// ([c.math]/1: the C++ functions' semantics are the C library's; the suffixed names and the
// overloads denote the same functions.)
#include <cmath>
#include <cstring>
#include <type_traits>
#include "check.hpp"

template <class T>
bool same(T a, T b) {
  if (std::isnan(a) && std::isnan(b)) return true;
  return std::memcmp(&a, &b, sizeof(T)) == 0 || a == b; // (a == b also for padding in long double)
}

#define UNARY(name)                                                                              \
  static_assert(std::is_same_v<decltype(std::name##f(1.0f)), float>);                            \
  static_assert(std::is_same_v<decltype(std::name##l(1.0L)), long double>);                      \
  for (float x : fx) CHECK(same(std::name##f(x), std::name(x)));                                  \
  for (long double x : lx) CHECK(same(std::name##l(x), std::name(x)));                            \
  CHECK(same(std::name##f(0.3), std::name(static_cast<float>(0.3))))

#define BINARY(name)                                                                             \
  static_assert(std::is_same_v<decltype(std::name##f(1.0f, 2.0f)), float>);                      \
  static_assert(std::is_same_v<decltype(std::name##l(1.0L, 2.0L)), long double>);                \
  for (float x : fx)                                                                             \
    for (float y : fx) CHECK(same(std::name##f(x, y), std::name(x, y)));                          \
  for (long double x : lx)                                                                       \
    for (long double y : lx) CHECK(same(std::name##l(x, y), std::name(x, y)))

int main() {
  const float fx[] = {0.0f, -0.0f, 0.25f, 0.5f, 1.0f, 1.5f, 2.75f, -3.5f, 10.0f};
  const long double lx[] = {0.0L, -0.0L, 0.25L, 0.5L, 1.0L, 1.5L, 2.75L, -3.5L, 10.0L};
  UNARY(acos);
  UNARY(asin);
  UNARY(atan);
  UNARY(cos);
  UNARY(sin);
  UNARY(tan);
  UNARY(acosh);
  UNARY(asinh);
  UNARY(atanh);
  UNARY(cosh);
  UNARY(sinh);
  UNARY(tanh);
  UNARY(exp);
  UNARY(exp2);
  UNARY(expm1);
  UNARY(log);
  UNARY(log10);
  UNARY(log1p);
  UNARY(log2);
  UNARY(logb);
  UNARY(cbrt);
  UNARY(fabs);
  UNARY(sqrt);
  UNARY(erf);
  UNARY(erfc);
  UNARY(lgamma);
  UNARY(tgamma);
  UNARY(ceil);
  UNARY(floor);
  UNARY(nearbyint);
  UNARY(rint);
  UNARY(round);
  UNARY(trunc);
  UNARY(nextup);
  UNARY(nextdown);
  BINARY(atan2);
  BINARY(hypot);
  BINARY(fmod);
  BINARY(remainder);
  BINARY(copysign);
  BINARY(nextafter);
  BINARY(fdim);
  BINARY(fmax);
  BINARY(fmin);

  // Functions with other parameter or return types.
  static_assert(std::is_same_v<decltype(std::ilogbf(1.0f)), int> && std::is_same_v<decltype(std::ilogbl(1.0L)), int>);
  static_assert(std::is_same_v<decltype(std::lrintf(1.0f)), long> && std::is_same_v<decltype(std::llroundl(1.0L)), long long>);
  for (float x : fx) {
    CHECK(x == 0 || std::ilogbf(x) == std::ilogb(x));
    CHECK(std::lrintf(x) == std::lrint(x) && std::llrintf(x) == std::llrint(x));
    CHECK(std::lroundf(x) == std::lround(x) && std::llroundf(x) == std::llround(x));
    int e1 = 0, e2 = 0;
    CHECK(same(std::frexpf(x, &e1), std::frexp(x, &e2)) && e1 == e2);
    float i1 = 0, i2 = 0;
    CHECK(same(std::modff(x, &i1), std::modf(x, &i2)) && same(i1, i2));
    CHECK(same(std::ldexpf(x, 3), std::ldexp(x, 3)) && same(std::scalbnf(x, -2), std::scalbn(x, -2)));
    CHECK(same(std::scalblnf(x, 4L), std::scalbln(x, 4L)));
    int q1 = 0, q2 = 0;
    CHECK(same(std::remquof(x, 0.75f, &q1), std::remquo(x, 0.75f, &q2)) && q1 == q2);
    CHECK(same(std::fmaf(x, 2.0f, 0.5f), std::fma(x, 2.0f, 0.5f)));
    CHECK(same(std::nexttowardf(x, 100.0L), std::nexttoward(x, 100.0L)));
  }
  for (long double x : lx) {
    CHECK(x == 0 || std::ilogbl(x) == std::ilogb(x));
    CHECK(std::lrintl(x) == std::lrint(x) && std::llrintl(x) == std::llrint(x));
    CHECK(std::lroundl(x) == std::lround(x) && std::llroundl(x) == std::llround(x));
    int e1 = 0, e2 = 0;
    CHECK(same(std::frexpl(x, &e1), std::frexp(x, &e2)) && e1 == e2);
    long double i1 = 0, i2 = 0;
    CHECK(same(std::modfl(x, &i1), std::modf(x, &i2)) && same(i1, i2));
    CHECK(same(std::ldexpl(x, 3), std::ldexp(x, 3)) && same(std::scalbnl(x, -2), std::scalbn(x, -2)));
    CHECK(same(std::scalblnl(x, 4L), std::scalbln(x, 4L)));
    int q1 = 0, q2 = 0;
    CHECK(same(std::remquol(x, 0.75L, &q1), std::remquo(x, 0.75L, &q2)) && q1 == q2);
    CHECK(same(std::fmal(x, 2.0L, 0.5L), std::fma(x, 2.0L, 0.5L)));
    CHECK(same(std::nexttowardl(x, 100.0L), std::nexttoward(x, 100.0L)));
  }
  CHECK(std::isnan(std::nanf("")) && std::isnan(std::nanl("")));
  static_assert(std::is_same_v<decltype(std::nanf("")), float> && std::is_same_v<decltype(std::nanl("")), long double>);

  // [sf.cmath]: the special functions' f and l forms.
  static_assert(std::is_same_v<decltype(std::expintf(1.0f)), float> && std::is_same_v<decltype(std::betal(1.0L, 2.0L)), long double>);
  CHECK(same(std::assoc_laguerref(2, 1, 0.5f), std::assoc_laguerre(2, 1, 0.5f)));
  CHECK(same(std::assoc_laguerrel(2, 1, 0.5L), std::assoc_laguerre(2, 1, 0.5L)));
  CHECK(same(std::assoc_legendref(3, 1, 0.5f), std::assoc_legendre(3, 1, 0.5f)));
  CHECK(same(std::assoc_legendrel(3, 1, 0.5L), std::assoc_legendre(3, 1, 0.5L)));
  CHECK(same(std::betaf(1.5f, 2.5f), std::beta(1.5f, 2.5f)) && same(std::betal(1.5L, 2.5L), std::beta(1.5L, 2.5L)));
  CHECK(same(std::comp_ellint_1f(0.5f), std::comp_ellint_1(0.5f)) && same(std::comp_ellint_1l(0.5L), std::comp_ellint_1(0.5L)));
  CHECK(same(std::comp_ellint_2f(0.5f), std::comp_ellint_2(0.5f)) && same(std::comp_ellint_2l(0.5L), std::comp_ellint_2(0.5L)));
  CHECK(same(std::comp_ellint_3f(0.5f, 0.25f), std::comp_ellint_3(0.5f, 0.25f)));
  CHECK(same(std::comp_ellint_3l(0.5L, 0.25L), std::comp_ellint_3(0.5L, 0.25L)));
  CHECK(same(std::cyl_bessel_if(1.0f, 2.0f), std::cyl_bessel_i(1.0f, 2.0f)) && same(std::cyl_bessel_il(1.0L, 2.0L), std::cyl_bessel_i(1.0L, 2.0L)));
  CHECK(same(std::cyl_bessel_jf(1.0f, 2.0f), std::cyl_bessel_j(1.0f, 2.0f)) && same(std::cyl_bessel_jl(1.0L, 2.0L), std::cyl_bessel_j(1.0L, 2.0L)));
  CHECK(same(std::cyl_bessel_kf(1.0f, 2.0f), std::cyl_bessel_k(1.0f, 2.0f)) && same(std::cyl_bessel_kl(1.0L, 2.0L), std::cyl_bessel_k(1.0L, 2.0L)));
  CHECK(same(std::cyl_neumannf(1.0f, 2.0f), std::cyl_neumann(1.0f, 2.0f)) && same(std::cyl_neumannl(1.0L, 2.0L), std::cyl_neumann(1.0L, 2.0L)));
  CHECK(same(std::ellint_1f(0.5f, 1.0f), std::ellint_1(0.5f, 1.0f)) && same(std::ellint_1l(0.5L, 1.0L), std::ellint_1(0.5L, 1.0L)));
  CHECK(same(std::ellint_2f(0.5f, 1.0f), std::ellint_2(0.5f, 1.0f)) && same(std::ellint_2l(0.5L, 1.0L), std::ellint_2(0.5L, 1.0L)));
  CHECK(same(std::ellint_3f(0.5f, 0.25f, 1.0f), std::ellint_3(0.5f, 0.25f, 1.0f)));
  CHECK(same(std::ellint_3l(0.5L, 0.25L, 1.0L), std::ellint_3(0.5L, 0.25L, 1.0L)));
  CHECK(same(std::expintf(1.5f), std::expint(1.5f)) && same(std::expintl(1.5L), std::expint(1.5L)));
  CHECK(same(std::hermitef(3, 0.5f), std::hermite(3, 0.5f)) && same(std::hermitel(3, 0.5L), std::hermite(3, 0.5L)));
  CHECK(same(std::laguerref(3, 0.5f), std::laguerre(3, 0.5f)) && same(std::laguerrel(3, 0.5L), std::laguerre(3, 0.5L)));
  CHECK(same(std::legendref(3, 0.5f), std::legendre(3, 0.5f)) && same(std::legendrel(3, 0.5L), std::legendre(3, 0.5L)));
  CHECK(same(std::riemann_zetaf(3.0f), std::riemann_zeta(3.0f)) && same(std::riemann_zetal(3.0L), std::riemann_zeta(3.0L)));
  CHECK(same(std::sph_besself(2, 1.5f), std::sph_bessel(2, 1.5f)) && same(std::sph_bessell(2, 1.5L), std::sph_bessel(2, 1.5L)));
  CHECK(same(std::sph_legendref(3, 1, 0.5f), std::sph_legendre(3, 1, 0.5f)));
  CHECK(same(std::sph_legendrel(3, 1, 0.5L), std::sph_legendre(3, 1, 0.5L)));
  CHECK(same(std::sph_neumannf(2, 1.5f), std::sph_neumann(2, 1.5f)) && same(std::sph_neumannl(2, 1.5L), std::sph_neumann(2, 1.5L)));
  // The f forms convert a double argument to float.
  CHECK(same(std::expintf(1.1), std::expint(static_cast<float>(1.1))));
  return 0;
}
