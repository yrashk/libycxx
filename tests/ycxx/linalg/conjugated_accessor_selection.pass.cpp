// [linalg.conj.conjugated]/1: the accessor A of conjugated(a) is
//   (1.1) the nested accessor if Accessor is a conjugated_accessor (conjugating twice undoes it),
//   (1.2) Accessor if the element type is arithmetic,
//   (1.3) conjugated_accessor<Accessor> if conj(E) is valid (ADL, with a deleted
//         template<class U> U conj(const U&) in scope, so only a non-template or better match
//         counts),
//   (1.4) otherwise Accessor;
// /2: (2.2) returns a itself when A is Accessor.
// [linalg.conj.conjugatedaccessor]/1: element_type is const decltype(conj-if-needed(declval<
// NestedAccessor::element_type>())), reference is its non-const type, data_handle_type is the
// nested one, offset_policy wraps the nested offset_policy; /4: the converting constructor is
// explicit iff the nested accessors are not convertible; /6: access returns conj-if-needed of the
// nested element; /7: offset is the nested accessor's offset.
// [linalg.helpers.conj]/1: conj-if-needed(E) is conj(E) for a non-arithmetic type with a valid
// conj(E), otherwise E.
#include <linalg>
#include <complex>
#include <cstddef>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;
using std::extents;

namespace user {
// a "complex" type with an ADL conj
struct gauss {
  int re, im;
  friend constexpr bool operator==(gauss, gauss) = default;
};
constexpr gauss conj(const gauss& g) { return {g.re, -g.im}; }
// a type without conj
struct plain {
  int v;
  friend constexpr bool operator==(plain, plain) = default;
};
// a type whose only conj is an unconstrained template: the deleted declaration in /1.3's
// context is an equally good candidate, so overload resolution is ambiguous and conj(E) is not
// valid
struct tmpl {
  int v;
};
template <class T>
constexpr T conj(const T& t) { return t; }
}  // namespace user

// an accessor whose offset_policy differs, to see that offset_policy is rewrapped
template <class T>
struct offset_acc {
  using element_type = T;
  using reference = T&;
  using data_handle_type = T*;
  using offset_policy = std::default_accessor<T>;
  constexpr reference access(T* p, std::size_t i) const noexcept { return p[i]; }
  constexpr T* offset(T* p, std::size_t i) const noexcept { return p + i; }
};

constexpr bool run() {
  // (1.2): arithmetic types are returned as is
  double d[3] = {1, -2, 3};
  std::mdspan<double, extents<int, 3>> rd(d);
  static_assert(std::is_same_v<decltype(la::conjugated(rd)), decltype(rd)>);
  if (la::conjugated(rd).data_handle() != d || la::conjugated(rd)[1] != -2) return false;
  std::mdspan<const int, extents<int, 3>> ci(nullptr);
  static_assert(std::is_same_v<decltype(la::conjugated(ci)), decltype(ci)>);

  // (1.3) via user ADL conj; access conjugates
  user::gauss g[2] = {{1, 2}, {3, -4}};
  std::mdspan<user::gauss, extents<int, 2>> mg(g);
  auto cg = la::conjugated(mg);
  using CA = la::conjugated_accessor<std::default_accessor<user::gauss>>;
  static_assert(std::is_same_v<decltype(cg), std::mdspan<const user::gauss, extents<int, 2>, std::layout_right, CA>>);
  static_assert(std::is_same_v<CA::element_type, const user::gauss>);
  static_assert(std::is_same_v<CA::reference, user::gauss>);
  static_assert(std::is_same_v<CA::data_handle_type, user::gauss*>);
  static_assert(std::is_same_v<CA::offset_policy, CA>);
  if (!(cg[0] == user::gauss{1, -2}) || !(cg[1] == user::gauss{3, 4})) return false;
  if (!(g[0] == user::gauss{1, 2})) return false;  // the data are not modified
  // (1.1) / (2.1): conjugating twice gives the original accessor type back
  auto ccg = la::conjugated(cg);
  static_assert(std::is_same_v<decltype(ccg), std::mdspan<user::gauss, extents<int, 2>>>);
  if (ccg.data_handle() != g || !(ccg[1] == user::gauss{3, -4})) return false;

  // (1.4): no conj at all, and a conj that only a template provides
  std::mdspan<user::plain, extents<int, 1>> mp(nullptr);
  static_assert(std::is_same_v<decltype(la::conjugated(mp)), decltype(mp)>);
  std::mdspan<user::tmpl, extents<int, 1>> mt(nullptr);
  static_assert(std::is_same_v<decltype(la::conjugated(mt)), decltype(mt)>);

  // offset_policy is conjugated_accessor<Nested::offset_policy>; offset is the nested offset
  using CO = la::conjugated_accessor<offset_acc<user::gauss>>;
  static_assert(std::is_same_v<CO::offset_policy, la::conjugated_accessor<std::default_accessor<user::gauss>>>);
  CO co{offset_acc<user::gauss>{}};
  if (co.offset(g, 1) != g + 1 || !(co.access(g, 1) == user::gauss{3, 4})) return false;

  // /4: converting constructor
  using CN = la::conjugated_accessor<std::default_accessor<const user::gauss>>;
  static_assert(std::is_convertible_v<CA, CN>);  // default_accessor<T> -> <const T> is implicit
  static_assert(!std::is_constructible_v<CA, CN>);
  CN cn = cg.accessor();
  if (!(cn.access(g, 0) == user::gauss{1, -2})) return false;
  return true;
}

int main() {
  CHECK(run());
  static_assert(run());

  // std::complex: conj via the standard overload; reference is a prvalue complex
  using C = std::complex<float>;
  C c[2] = {{1, 2}, {-3, 4}};
  std::mdspan<C, std::dextents<int, 1>> mc(c, 2);
  auto cc = la::conjugated(mc);
  using A = decltype(cc)::accessor_type;
  static_assert(std::is_same_v<A, la::conjugated_accessor<std::default_accessor<C>>>);
  static_assert(std::is_same_v<A::reference, C> && std::is_same_v<A::element_type, const C>);
  CHECK(cc[0] == C(1, -2) && cc[1] == C(-3, -4));
  static_assert(std::is_same_v<decltype(la::conjugated(cc)), decltype(mc)>);
  // conjugate_transposed is transposed(conjugated(a)) ([linalg.conjtransposed])
  C m[4] = {{1, 1}, {2, 2}, {3, 3}, {4, 4}};
  std::mdspan<C, extents<int, 2, 2>> M(m);
  auto ct = la::conjugate_transposed(M);
  CHECK(ct[0, 1] == C(3, -3) && ct[1, 0] == C(2, -2));
  static_assert(std::is_same_v<decltype(ct)::layout_type, std::layout_left>);
  return 0;
}
