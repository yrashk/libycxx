// [cmplx.over]/1-2: arg, conj, imag, norm, proj and real have additional constexpr overloads
// so that a floating-point argument of type T is effectively cast to complex<T> and an
// integer argument to complex<double>. /3: pow with one complex<T1> argument and another of
// type T2 or complex<T2> casts both to complex<common_type_t<T1, T3>>, T3 being double if T2
// is an integer type and T2 otherwise.
#include <complex>
#include <type_traits>

using std::complex;
using std::is_same_v;

static_assert(is_same_v<decltype(std::real(1.0f)), float>);
static_assert(is_same_v<decltype(std::real(1)), double>);
static_assert(is_same_v<decltype(std::imag(1.0L)), long double>);
static_assert(is_same_v<decltype(std::imag(1u)), double>);
static_assert(is_same_v<decltype(std::arg(1.0f)), float>);
static_assert(is_same_v<decltype(std::arg(1)), double>);
static_assert(is_same_v<decltype(std::norm(2.0f)), float>);
static_assert(is_same_v<decltype(std::norm(2L)), double>);
static_assert(is_same_v<decltype(std::conj(1.0f)), complex<float>>);
static_assert(is_same_v<decltype(std::conj(1)), complex<double>>);
static_assert(is_same_v<decltype(std::proj(1.0L)), complex<long double>>);
static_assert(is_same_v<decltype(std::proj(1)), complex<double>>);

static_assert(std::real(5) == 5.0 && std::imag(5) == 0.0 && std::norm(3) == 9.0);
static_assert(std::norm(-2.0f) == 4.0f && std::conj(2.0) == complex<double>(2, 0));
static_assert(std::arg(1.0) == 0.0 && std::proj(4) == complex<double>(4, 0));

static_assert(is_same_v<decltype(std::pow(complex<float>(), 2)), complex<double>>);
static_assert(is_same_v<decltype(std::pow(2, complex<float>())), complex<double>>);
static_assert(is_same_v<decltype(std::pow(complex<float>(), 2.0f)), complex<float>>);
static_assert(is_same_v<decltype(std::pow(complex<float>(), 2.0)), complex<double>>);
static_assert(is_same_v<decltype(std::pow(2.0L, complex<double>())), complex<long double>>);
static_assert(is_same_v<decltype(std::pow(complex<float>(), complex<long double>())), complex<long double>>);
static_assert(is_same_v<decltype(std::pow(complex<double>(), complex<double>())), complex<double>>);
static_assert(is_same_v<decltype(std::pow(complex<long double>(), 2)), complex<long double>>);
