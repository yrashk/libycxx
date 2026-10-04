// [complex.syn], [complex.transcendentals], [complex.value.ops]: in C++26 (P1383R2) abs, arg,
// polar and all the transcendental functions on complex are constexpr, so they can be used in
// constant expressions (here with arguments whose results raise no floating-point exception).
#include <complex>

using C = std::complex<double>;

constexpr bool near(C a, C b) {
  C d = a - b;
  return d.real() * d.real() + d.imag() * d.imag() < 1e-24;
}
constexpr bool near(double a, double b) { return (a - b) * (a - b) < 1e-24; }

static_assert(near(std::exp(C(0, 0)), C(1, 0)));
static_assert(near(std::log(C(1, 0)), C(0, 0)));
static_assert(near(std::log10(C(10, 0)), C(1, 0)));
static_assert(near(std::sqrt(C(4, 0)), C(2, 0)));
static_assert(near(std::sqrt(C(0, 2)), C(1, 1)));
static_assert(near(std::pow(C(2, 0), C(3, 0)), C(8, 0)));
static_assert(near(std::pow(C(2, 0), 2.0), C(4, 0)));
static_assert(near(std::pow(2.0, C(2, 0)), C(4, 0)));
static_assert(near(std::sin(C(0, 0)), C(0, 0)));
static_assert(near(std::cos(C(0, 0)), C(1, 0)));
static_assert(near(std::tan(C(0, 0)), C(0, 0)));
static_assert(near(std::sinh(C(0, 0)), C(0, 0)));
static_assert(near(std::cosh(C(0, 0)), C(1, 0)));
static_assert(near(std::tanh(C(0, 0)), C(0, 0)));
static_assert(near(std::asin(C(0, 0)), C(0, 0)));
static_assert(near(std::acos(C(1, 0)), C(0, 0)));
static_assert(near(std::atan(C(0, 0)), C(0, 0)));
static_assert(near(std::asinh(C(0, 0)), C(0, 0)));
static_assert(near(std::acosh(C(1, 0)), C(0, 0)));
static_assert(near(std::atanh(C(0, 0)), C(0, 0)));
static_assert(near(std::abs(C(3, 4)), 5));
static_assert(near(std::arg(C(1, 0)), 0));
static_assert(near(std::polar(2.0, 0.0), C(2, 0)));
static_assert(near(std::exp(std::complex<float>(0, 0)), std::complex<float>(1, 0)));

int main() { return 0; }
