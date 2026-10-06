// Nesting the in-place transformations:
// [linalg.conj.conjugated]/1-/2: conjugated of a scaled view wraps its scaled_accessor in a
// conjugated_accessor (1.3: the element type, a complex, has conj); conjugating that again
// unwraps it (1.1, 2.1). [linalg.scaled.scaled]/2-/3: scaled of a conjugated view wraps the
// conjugated_accessor in a scaled_accessor.
// [linalg.conj.conjugatedaccessor]/1, /6: element_type is const decltype(conj-if-needed(nested
// element)), access returns conj-if-needed(NestedAccessor::element_type(nested access)).
// [linalg.scaled.scaledaccessor]/6: access is scaling_factor() * NestedAccessor::element_type(...).
// [linalg.transp.transposed]/4: transposed keeps the accessor.
// So conjugated(scaled(a, x))[i] is conj(a * x[i]) and scaled(a, conjugated(x))[i] is
// a * conj(x[i]).
#include <linalg>
#include <complex>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;
using C = std::complex<double>;
using DA = std::default_accessor<C>;

int main() {
  C d[3] = {{1, 2}, {-1, 1}, {0, -3}};
  std::mdspan<C, std::extents<int, 3>> x(d);
  const C a(2, 1);

  auto cs = la::conjugated(la::scaled(a, x));
  using CS = la::conjugated_accessor<la::scaled_accessor<C, DA>>;
  static_assert(std::is_same_v<decltype(cs)::accessor_type, CS>);
  static_assert(std::is_same_v<CS::element_type, const C> && std::is_same_v<CS::reference, C>);
  for (int i = 0; i < 3; ++i) CHECK(cs[i] == std::conj(a * d[i]));
  CHECK(cs.accessor().nested_accessor().scaling_factor() == a);

  auto back = la::conjugated(cs);  // unwraps to the scaled view
  static_assert(std::is_same_v<decltype(back)::accessor_type, la::scaled_accessor<C, DA>>);
  for (int i = 0; i < 3; ++i) CHECK(back[i] == a * d[i]);

  auto sc = la::scaled(a, la::conjugated(x));
  static_assert(std::is_same_v<decltype(sc)::accessor_type, la::scaled_accessor<C, la::conjugated_accessor<DA>>>);
  for (int i = 0; i < 3; ++i) CHECK(sc[i] == a * std::conj(d[i]));

  // a real scaling factor of a conjugated complex view, and a complex factor of a real view
  auto rc = la::scaled(3.0, la::conjugated(x));
  static_assert(std::is_same_v<decltype(rc)::element_type, const C>);
  CHECK(rc[0] == C(3, -6));
  double r[2] = {1, -2};
  std::mdspan<double, std::extents<int, 2>> xr(r);
  auto cr = la::conjugated(la::scaled(a, xr));  // complex elements: conjugated
  static_assert(std::is_same_v<decltype(cr)::accessor_type, la::conjugated_accessor<la::scaled_accessor<C, std::default_accessor<double>>>>);
  CHECK(cr[1] == std::conj(a * -2.0));
  auto rr = la::conjugated(la::scaled(2.0, xr));  // real elements: unchanged
  static_assert(std::is_same_v<decltype(rr)::accessor_type, la::scaled_accessor<double, std::default_accessor<double>>>);

  // in a matrix, through transposed
  C m[4] = {{1, 1}, {2, 0}, {0, 1}, {1, -1}};
  std::mdspan<C, std::extents<int, 2, 2>> M(m);
  auto t = la::transposed(la::conjugated(la::scaled(a, M)));
  static_assert(std::is_same_v<decltype(t)::accessor_type, CS>);
  CHECK(t[1, 0] == std::conj(a * m[1]) && t[0, 1] == std::conj(a * m[2]));
  return 0;
}
