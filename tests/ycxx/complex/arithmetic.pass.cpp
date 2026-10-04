// [complex.member.ops], [complex.ops]: compound assignment with T and with complex<X>, the
// binary operators for complex op complex, complex op T and T op complex (each "Returns:
// complex<T>(lhs) op= rhs"), unary + and -, and == with "the imaginary part assumed to be
// T()" for a T operand. All are constexpr.
#include <complex>
#include <cmath>
#include <type_traits>
#include "check.hpp"

using C = std::complex<double>;

constexpr bool compound() {
  C z(1, 2);
  if (&(z += 3.0) != &z || z != C(4, 2)) return false;
  if (&(z -= 1.0) != &z || z != C(3, 2)) return false;
  if (&(z *= 2.0) != &z || z != C(6, 4)) return false;
  if (&(z /= 2.0) != &z || z != C(3, 2)) return false;
  if (&(z += C(1, 1)) != &z || z != C(4, 3)) return false;
  if (&(z -= std::complex<float>(1, 1)) != &z || z != C(3, 2)) return false;  // template<class X>
  if (&(z *= C(1, 1)) != &z || z != C(1, 5)) return false;                     // (3+2i)(1+i) = 1+5i
  if (&(z /= C(1, 1)) != &z || z != C(3, 2)) return false;                     // (1+5i)/(1+i) = 3+2i
  z *= std::complex<long double>(0, 1);                                        // times i
  if (z != C(-2, 3)) return false;
  return true;
}
static_assert(compound());

constexpr bool binary() {
  const C a(1, 2), b(3, 4);
  if (a + b != C(4, 6) || a - b != C(-2, -2) || a * b != C(-5, 10)) return false;
  if (a + 1.0 != C(2, 2) || 1.0 + a != C(2, 2)) return false;
  if (a - 1.0 != C(0, 2) || 1.0 - a != C(0, -2)) return false;
  if (a * 2.0 != C(2, 4) || 2.0 * a != C(2, 4)) return false;
  if (a / 2.0 != C(0.5, 1) || C(-5, 10) / b != a) return false;
  if (2.0 / C(1, 1) != C(1, -1)) return false;
  if (+a != a || -a != C(-1, -2)) return false;
  if (!(C(3, 0) == 3.0) || !(3.0 == C(3, 0)) || C(3, 1) == 3.0 || !(C(3, 1) != 3.0)) return false;
  if (!(a == C(1, 2)) || a != C(1, 2) || a == b) return false;
  return true;
}
static_assert(binary());

static_assert(std::is_same_v<decltype(C() + 1.0), C>);
static_assert(std::is_same_v<decltype(1.0f * std::complex<float>()), std::complex<float>>);
static_assert(std::is_same_v<decltype(-C()), C>);
static_assert(std::is_same_v<decltype(C() == 1.0), bool>);

// The binary operators are templates on T: an int operand does not deduce.
template <class A, class B> concept addable = requires(A a, B b) { a + b; };
static_assert(addable<C, double> && addable<double, C> && addable<C, C>);
static_assert(!addable<C, int> && !addable<int, C>);
static_assert(!addable<C, std::complex<float>>);

int main() {
  CHECK(compound() && binary());
  // Run-time: division of general values (within rounding).
  std::complex<float> q = std::complex<float>(7, -4) / std::complex<float>(2, 1);  // = 2 - 3i
  CHECK(std::fabs(q.real() - 2) < 1e-6f && std::fabs(q.imag() + 3) < 1e-6f);
  std::complex<long double> w(0.5L, 0.25L);
  w *= w;  // 0.25 - 0.0625 + 0.25i
  CHECK(w == std::complex<long double>(0.1875L, 0.25L));
  return 0;
}
