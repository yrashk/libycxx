// [complex.literals]: operator""il, ""i and ""if (from long double and unsigned long long)
// return complex<long double>, complex<double> and complex<float> with real part 0 and the
// literal as imaginary part. They live in std::literals::complex_literals (both inline
// namespaces) and are constexpr.
#include <complex>
#include <cmath>
#include <type_traits>
#include "check.hpp"

void via_literals() {
  using namespace std::literals;
  static_assert(std::is_same_v<decltype(2i), std::complex<double>>);
  static_assert((2.5i).imag() == 2.5);
}
void via_complex_literals() {
  using namespace std::complex_literals;
  static_assert(std::is_same_v<decltype(1if), std::complex<float>>);
}
void via_both() {
  using namespace std::literals::complex_literals;
  static_assert(std::is_same_v<decltype(1.0il), std::complex<long double>>);
}

using namespace std::complex_literals;
static_assert(std::is_same_v<decltype(3i), std::complex<double>>);
static_assert(std::is_same_v<decltype(3.0i), std::complex<double>>);
static_assert(std::is_same_v<decltype(3if), std::complex<float>>);
static_assert(std::is_same_v<decltype(3.0if), std::complex<float>>);
static_assert(std::is_same_v<decltype(3il), std::complex<long double>>);
static_assert(std::is_same_v<decltype(3.0il), std::complex<long double>>);
static_assert(3i == std::complex<double>(0, 3));
static_assert(0.5if == std::complex<float>(0, 0.5f));
static_assert(7il == std::complex<long double>(0, 7));
static_assert((1.0 + 2i) == std::complex<double>(1, 2));
static_assert((2i * 2i) == std::complex<double>(-4, 0));
static_assert((0.1if).imag() == 0.1f);   // static_cast<float>(0.1L)
static_assert((0.1il).imag() == 0.1L);

int main() {
  auto z = 1.5 - 2.5i;
  CHECK(z.real() == 1.5 && z.imag() == -2.5);
  auto f = 1.0f + 1if;
  CHECK(f == std::complex<float>(1, 1));
  CHECK((0i).real() == 0 && !std::signbit((0i).real()));
  return 0;
}
