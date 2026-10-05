// [complex.h.syn]: "The header <complex.h> behaves as if it simply includes the header
// <complex>" (Note 1: the names of <complex> are not placed into the global namespace).
// [tgmath.h.syn]: "The header <tgmath.h> behaves as if it simply includes the headers <cmath>
// and <complex>" (Note 1: the type-generic macros' overloads are already provided by the
// "sufficient additional overloads" of <complex> and <cmath>).
// So after either header, std::complex, its literals and functions, and (after <tgmath.h>) the
// <cmath> overloads are usable, and neither header defines a `complex` or `I` macro that would
// break C++ code such as the declarations below. [iso646.h.syn]: <iso646.h> is empty (and,
// or, ... are keywords).
#include <complex.h>
#include <iso646.h>
#include <tgmath.h>

#include "check.hpp"

template <class A, class B>
constexpr bool same = __is_same(A, B);

int complex = 1;  // an ordinary name: no `complex` macro
int I = 2;        // no `I` macro

int main() {
  std::complex<double> z(3.0, 4.0);
  CHECK(std::abs(z) == 5.0);
  using namespace std::complex_literals;
  auto w = 2.0 + 1.0i;
  static_assert(same<decltype(w), std::complex<double>>);
  CHECK(w.imag() == 1.0 and w.real() == 2.0);
  // <cmath> overloads via <tgmath.h>, for each floating-point type and for integers.
  static_assert(same<decltype(std::sqrt(4.0f)), float>);
  static_assert(same<decltype(std::sqrt(4)), double>);
  static_assert(same<decltype(std::sqrt(z)), std::complex<double>>);
  CHECK(std::sqrt(4.0) == 2.0 && std::pow(2.0, 3) == 8.0);
  CHECK(std::sqrt(std::complex<double>(-4.0, 0.0)).imag() == 2.0);
  return complex + I == 3 ? 0 : 1;
}
