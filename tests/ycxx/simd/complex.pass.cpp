// [simd.general]/2.3: complex<T> for a vectorizable floating-point T is vectorizable.
// [simd.overview]/3: real-type is rebind_t<T::value_type, basic_vec>. [simd.ctor]/20-21:
// basic_vec(reals, imags = {}) builds value_type(reals[i], imags[i]).
// [simd.complex.access]: real()/imag() return real-type, real(v)/imag(v) replace the parts.
// [simd.complex.math]: real, imag, abs, arg, norm (rebind_t<T::value_type, V>), conj and the
// other <complex> functions element-wise.
#include <simd>
#include <cmath>
#include <complex>
#include <type_traits>
#include "check.hpp"

namespace simd = std::simd;
using C = std::complex<double>;
using VC = simd::vec<C, 4>;
using VR = simd::vec<double, 4>;

int main() {
  static_assert(VC::size() == 4 && std::is_same_v<VC::value_type, C>);
  VR re([](int i) { return double(i); });
  VR im([](int i) { return double(-i); });
  VC z(re, im);
  CHECK(z[2] == C(2, -2));
  VC onlyre(re);
  CHECK(onlyre[3] == C(3, 0));
  static_assert(std::is_same_v<decltype(z.real()), VR> && std::is_same_v<decltype(z.imag()), VR>);
  CHECK(z.real()[1] == 1 && z.imag()[1] == -1);
  z.real(VR(5.0));
  CHECK(z[0] == C(5, 0) && z[3] == C(5, -3));
  z.imag(VR(4.0));
  CHECK(z[1] == C(5, 4));
  VC b = C(1, 1);
  CHECK((z + b)[0] == C(6, 5) && (z * b)[0] == C(1, 9));
  CHECK(simd::all_of(z == z) && simd::none_of(z != z));
  static_assert(std::is_same_v<decltype(simd::abs(z)), VR> && std::is_same_v<decltype(simd::norm(z)), VR>);
  CHECK(std::fabs(simd::abs(z)[0] - std::sqrt(41.0)) < 1e-12);
  CHECK(simd::norm(z)[0] == 41.0);
  CHECK(simd::real(z)[2] == 5.0 && simd::imag(z)[2] == 4.0);
  CHECK(simd::conj(z)[0] == C(5, -4));
  CHECK(std::fabs(simd::arg(VC(C(0, 1)))[0] - std::acos(0.0)) < 1e-12);
  auto e = simd::exp(VC(C(0, 0)));
  CHECK(e[0] == C(1, 0));
  return 0;
}
