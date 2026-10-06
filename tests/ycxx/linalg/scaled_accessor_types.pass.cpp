// [linalg.scaled.scaledaccessor]/1: scaled_accessor<S, NA>::element_type is
// const decltype(declval<S>() * declval<NA::element_type>()), reference its non-const type,
// data_handle_type NA's, offset_policy scaled_accessor<S, NA::offset_policy>.
// /3-/4: the converting constructor copies the scaling factor and the nested accessor and is
// explicit iff the nested accessors are not convertible; /6: access(p, i) is
// scaling_factor() * NA::element_type(nested.access(p, i)) -- the factor on the left; /7: offset
// is the nested accessor's.
// [linalg.scaled.scaled]/2-/3: scaled(alpha, x) has accessor scaled_accessor<ScalingFactor,
// Accessor> (no folding of nested scalings), the same data handle, extents and mapping.
#include <linalg>
#include <complex>
#include <cstddef>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;
using std::extents;

// 2 x 2 integer matrices: multiplication does not commute
struct mat {
  int a, b, c, d;
  friend constexpr mat operator*(const mat& x, const mat& y) {
    return {x.a * y.a + x.b * y.c, x.a * y.b + x.b * y.d, x.c * y.a + x.d * y.c, x.c * y.b + x.d * y.d};
  }
  friend constexpr bool operator==(const mat&, const mat&) = default;
};

constexpr bool run() {
  // element type follows the product's type
  float f[3] = {1.5f, 2, -3};
  std::mdspan<float, extents<int, 3>> vf(f);
  auto s1 = la::scaled(2, vf);  // int * float -> float
  static_assert(std::is_same_v<decltype(s1)::element_type, const float>);
  static_assert(std::is_same_v<decltype(s1)::reference, float>);
  auto s2 = la::scaled(2.0, vf);  // double * float -> double
  static_assert(std::is_same_v<decltype(s2)::element_type, const double>);
  static_assert(std::is_same_v<decltype(s2)::accessor_type, la::scaled_accessor<double, std::default_accessor<float>>>);
  static_assert(std::is_same_v<decltype(s2)::extents_type, extents<int, 3>>);
  if (s1[0] != 3.0f || s2[2] != -6.0 || s2.data_handle() != f) return false;
  if (s2.accessor().scaling_factor() != 2.0) return false;
  if (!(s2.mapping() == vf.mapping())) return false;

  // nested scaling is not folded: scaled_accessor<S2, scaled_accessor<S1, A>>
  auto s3 = la::scaled(3L, la::scaled(2, vf));
  using SA = la::scaled_accessor<long, la::scaled_accessor<int, std::default_accessor<float>>>;
  static_assert(std::is_same_v<decltype(s3)::accessor_type, SA>);
  static_assert(std::is_same_v<SA::element_type, const float>);
  if (s3[1] != 12.0f || s3.accessor().nested_accessor().scaling_factor() != 2) return false;

  // the factor multiplies from the left
  mat m[2] = {{1, 1, 0, 1}, {0, 1, 1, 0}};
  mat alpha{2, 0, 1, 1};
  std::mdspan<mat, extents<int, 2>> vm(m);
  auto sm = la::scaled(alpha, vm);
  if (!(sm[0] == alpha * m[0]) || sm[0] == m[0] * alpha) return false;
  if (!(sm[1] == alpha * m[1])) return false;

  // converting constructor; offset_policy; offset
  using A1 = la::scaled_accessor<double, std::default_accessor<float>>;
  using A2 = la::scaled_accessor<double, std::default_accessor<const float>>;
  static_assert(std::is_convertible_v<A1, A2> && !std::is_constructible_v<A1, A2>);
  static_assert(std::is_same_v<A1::offset_policy, A1>);
  static_assert(std::is_same_v<A1::data_handle_type, float*>);
  A2 a2 = s2.accessor();
  if (a2.scaling_factor() != 2.0 || a2.access(f, 1) != 4.0) return false;
  if (s2.accessor().offset(f, 2) != f + 2) return false;
  // a scaled view of a const view
  std::mdspan<const float, extents<int, 3>> cvf(f);
  if (la::scaled(-1.0f, cvf)[0] != -1.5f) return false;
  return true;
}

int main() {
  CHECK(run());
  static_assert(run());
  // complex factor times real elements: complex element type
  double d[2] = {1, 2};
  std::mdspan<double, extents<int, 2>> v(d);
  auto sc = la::scaled(std::complex<double>(0, 1), v);
  static_assert(std::is_same_v<decltype(sc)::reference, std::complex<double>>);
  CHECK(sc[1] == std::complex<double>(0, 2));
  // and the scaled view feeds an algorithm: dot(scaled(2, x), y)
  double y[2] = {3, 4};
  CHECK(la::dot(la::scaled(2.0, v), std::mdspan<double, extents<int, 2>>(y)) == 22.0);
  return 0;
}
