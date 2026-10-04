// [cmath.syn]: sqrt, fma, fmod, remainder, remquo and hypot are constexpr (P0533R9, P1383R2).
// [library.c]/3: evaluated as a core constant expression, a C library call has the semantics of
// ISO/IEC 9899:2024 Annex F, and is a non-constant library call only if it raises a
// floating-point exception other than FE_INEXACT. Annex F (F.3) binds sqrt, fma and remainder to
// the IEC 60559 operations squareRoot, fusedMultiplyAdd and remainder, which are correctly
// rounded (remainder exact); F.10.7.1: fmod's result is exact. So these constant expressions
// have exactly the values below (computed with exact rational arithmetic), even when the result
// is inexact (sqrt(2)), when a non-fused evaluation would lose the result (fma with
// cancellation) or overflow (fma(max, 2, -max) raises no exception, so it is a constant
// expression), and when the quotient is huge (fmod/remainder of max by 0.1 or 3).
// remquo (7.12.10.3): the quotient has the sign of x/y and is congruent modulo 2^n (n >= 3) to
// the integral quotient. hypot (7.12.7.4) computes "without undue overflow or underflow": with
// a representable result no exception other than inexact is raised, so it is a constant
// expression too (the result is not required to be correctly rounded: checked to 2 ulp).
#include <cmath>
#include <limits>
#include "check.hpp"

template <class T>
struct Expect {
  T sqrt2, sqrt3, sqrtmax, fma_tenth, fma_eps, fma_third, fmod_tenth, rem_tenth;
  int rem_tenth_q8;
  T fmod3, rem3;
  int rem3_q8;
};

template <class T>
constexpr bool check(const Expect<T>& x) {
  using L = std::numeric_limits<T>;
  const T eps = L::epsilon(), mx = L::max(), dmin = L::denorm_min();
  const T tenth = T(1) / T(10), third = T(1) / T(3);  // correctly rounded divisions
  if (std::sqrt(T(2)) != x.sqrt2 || std::sqrt(T(3)) != x.sqrt3 || std::sqrt(mx) != x.sqrtmax) return false;
  if (std::sqrt(T(1) + eps) != T(1)) return false;                // just below the midpoint
  if (std::sqrt(T(1) - eps / 2) != T(1) - eps / 2) return false;  // the largest value below 1
  if (std::sqrt(dmin * 4) != std::sqrt(dmin) * 2) return false;
  if (std::fma(tenth, T(10), T(-1)) != x.fma_tenth) return false;
  if (std::fma(T(1) + eps, T(1) - eps, T(-1)) != x.fma_eps) return false;
  if (std::fma(third, T(3), T(-1)) != x.fma_third) return false;
  if (std::fma(mx, T(2), -mx) != mx) return false;  // no intermediate overflow
  if (std::fma(-mx, T(0.5), mx) != mx / 2) return false;
  if (std::fmod(mx, tenth) != x.fmod_tenth || std::fmod(-mx, tenth) != -x.fmod_tenth) return false;
  if (std::remainder(mx, tenth) != x.rem_tenth) return false;
  if (std::fmod(mx, T(3)) != x.fmod3 || std::remainder(mx, T(3)) != x.rem3) return false;
  if (std::remainder(T(2.5), T(1)) != T(0.5) || std::remainder(T(3.5), T(1)) != T(-0.5)) return false;
  // exact subnormal results raise no underflow
  if (std::fmod(dmin * 3, dmin * 2) != dmin) return false;
  if (std::remainder(dmin * 5, dmin * 2) != dmin) return false;   // 2.5 -> 2 (ties to even)
  if (std::remainder(dmin * 7, dmin * 2) != -dmin) return false;  // 3.5 -> 4
  {
    unsigned r = 1;
    for (int i = 0; i < L::digits - 1; ++i) r = (r * 2) % 3;
    if (std::fmod(L::min(), dmin * 3) != dmin * T(r)) return false;
  }
  // hypot without undue overflow/underflow
  T h = std::hypot(mx / 2, mx / 2);
  T expect_h = mx / 2 * x.sqrt2;  // the exact result rounded twice; within 2 ulp of the value
  if (!(std::fabs(h - expect_h) <= expect_h * eps * 2)) return false;
  constexpr int s = L::min_exponent < -10000 ? -9000 : (L::min_exponent < -1000 ? -600 : -100);
  T small = std::hypot(std::ldexp(T(3), s), std::ldexp(T(4), s));
  T five = std::ldexp(T(5), s);
  if (!(std::fabs(small - five) <= five * eps * 2)) return false;
  return true;
}

template <class T>
constexpr bool check_remquo(const Expect<T>& x) {
  using L = std::numeric_limits<T>;
  const T mx = L::max(), tenth = T(1) / T(10);
  int q = -100;
  if (std::remquo(mx, tenth, &q) != x.rem_tenth || q < 0 || q % 8 != x.rem_tenth_q8) return false;
  if (std::remquo(-mx, T(3), &q) != -x.rem3 || q > 0 || (-q) % 8 != x.rem3_q8) return false;
  if (std::remquo(T(-7), T(2), &q) != T(1) || q >= 0 || (-q) % 8 != 4) return false;  // -3.5 -> -4
  return true;
}

constexpr Expect<float> ef{0xb504f3p-23f, 0xddb3d7p-23f, 0xffffffp+40f, 0x1p-26f, -0x1p-46f, 0x1p-25f,
                           0x666669p-27f, -0x199999p-25f, 4, 0.0f, 0.0f, 0};
constexpr Expect<double> ed{0x16a09e667f3bcdp-52, 0xddb3d742c2655p-51, 0x1fffffffffffffp+459, 0x1p-54, -0x1p-104,
                            -0x1p-54, 0xcccccccccccdp-52, -0x6666666666665p-55, 1, 2.0, -1.0, 3};
constexpr Expect<long double> el{0x2d413cccfe779921p-61L, 0x6ed9eba16132a9cfp-62L, 0xffffffffffffffffp+8128L,
                                 0x1p-66L, -0x1p-126L, 0x1p-65L, 0xccccccccccc2cccdp-67L, -0x5p-50L, 0, 0.0L, 0.0L, 0};

static_assert(std::numeric_limits<float>::digits == 24 && std::numeric_limits<double>::digits == 53);
static_assert(check(ef));
static_assert(check(ed));
// the long double values are those of the x87 80-bit format (this platform's)
static_assert(std::numeric_limits<long double>::digits != 64 || check(el));
static_assert(check_remquo(ef) && check_remquo(ed));
static_assert(std::numeric_limits<long double>::digits != 64 || check_remquo(el));

int main() {
  CHECK(check(ef) && check(ed) && check_remquo(ef) && check_remquo(ed));
  if constexpr (std::numeric_limits<long double>::digits == 64) CHECK(check(el) && check_remquo(el));
  // the same values at run time (the functions behave identically, [library.c]/2)
  volatile double two = 2.0;
  CHECK(std::sqrt(two) == ed.sqrt2);
  CHECK(std::fma(1.0 + 0x1p-52, 1.0 - 0x1p-52, -two / 2) == ed.fma_eps);
  return 0;
}
