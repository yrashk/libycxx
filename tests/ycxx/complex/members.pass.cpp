// [complex], [complex.members]: complex(re = T(), im = T()) has postcondition real() == re &&
// imag() == im; real(T)/imag(T) assign one component; value_type is T. The converting
// constructor from complex<X> is explicit unless the conversion rank of T is at least that of
// X. [complex.numbers.general]/2: specializations for cv-unqualified floating-point types are
// trivially copyable literal types.
#include <complex>
#include <type_traits>
#include "check.hpp"

template <class T>
constexpr bool basics() {
  std::complex<T> z;
  if (z.real() != T() || z.imag() != T()) return false;
  std::complex<T> a(T(1.5));
  if (a.real() != T(1.5) || a.imag() != 0) return false;
  std::complex<T> b(T(2), T(-3));
  if (b.real() != 2 || b.imag() != -3) return false;
  b.real(T(7));
  if (b.real() != 7 || b.imag() != -3) return false;
  b.imag(T(8));
  if (b.real() != 7 || b.imag() != 8) return false;
  std::complex<T> c = b;  // copy
  if (c != b) return false;
  c = T(4);  // operator=(const T&): imaginary part becomes 0
  if (c.real() != 4 || c.imag() != 0) return false;
  c = std::complex<float>(1.0f, 2.0f);  // operator=(const complex<X>&)
  if (c.real() != 1 || c.imag() != 2) return false;
  std::complex<T> d{T(1), T(2)};
  return d == std::complex<T>(1, 2);
}
static_assert(basics<float>() && basics<double>() && basics<long double>());

static_assert(std::is_same_v<std::complex<float>::value_type, float>);
static_assert(std::is_same_v<std::complex<long double>::value_type, long double>);
static_assert(std::is_trivially_copyable_v<std::complex<float>>);
static_assert(std::is_trivially_copyable_v<std::complex<double>>);
static_assert(std::is_trivially_copyable_v<std::complex<long double>>);
static_assert(std::is_same_v<decltype(std::complex<double>().real()), double>);
static_assert(std::is_same_v<decltype(std::complex<double>().real(1.0)), void>);

// Converting constructors.
static_assert(std::is_convertible_v<std::complex<float>, std::complex<double>>);
static_assert(std::is_convertible_v<std::complex<float>, std::complex<long double>>);
static_assert(std::is_convertible_v<std::complex<double>, std::complex<long double>>);
static_assert(!std::is_convertible_v<std::complex<double>, std::complex<float>>);
static_assert(!std::is_convertible_v<std::complex<long double>, std::complex<double>>);
static_assert(std::is_constructible_v<std::complex<float>, std::complex<long double>>);
static_assert(std::is_convertible_v<double, std::complex<double>>);  // complex(const T& re = T(), ...)
static_assert(std::is_nothrow_copy_constructible_v<std::complex<double>>);

constexpr std::complex<float> narrowed(std::complex<double>(0.5, -0.25));
static_assert(narrowed.real() == 0.5f && narrowed.imag() == -0.25f);
constexpr std::complex<long double> widened = std::complex<double>(3.0, 4.0);
static_assert(widened.real() == 3.0L && widened.imag() == 4.0L);

int main() {
  CHECK(basics<float>() && basics<double>() && basics<long double>());
  std::complex<double> z(1, 2);
  z.real(-1);
  CHECK(z == std::complex<double>(-1, 2));
  return 0;
}
