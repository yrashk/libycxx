// The converting constructors between the padded layouts and the other standard layouts, their
// constraints, their explicit-specifiers and the mappings they make:
//   [mdspan.layout.left.cons]/9-12 (layout_left from layout_left_padded: explicit iff the extents
//     are not convertible), [mdspan.layout.right.cons]/9-12 (the mirror image);
//   [mdspan.layout.leftpad.cons]/7-10 (from layout_left: explicit iff not convertible; the
//     padding is recomputed), /11-14 (from layout_stride: explicit unless rank 0), /15-19 (from
//     another layout_left_padded: explicit if the extents are not convertible, or if rank > 1 and
//     either this padding_value is static or the other's is dynamic), /20-23 (from
//     layout_right_padded or layout_right, rank 0 or 1 only: explicit iff not convertible);
//   [mdspan.layout.rightpad.cons], the mirror image;
//   [mdspan.layout.stride.cons]/6-9: from any unique strided mapping, implicit from the padded
//     layouts with convertible extents; [mdspan.layout.stride.obs]/7-9: == with them.
//   [mdspan.layout.leftpad.obs]/10-11: == between padded mappings compares the extents and, for
//     rank >= 2, stride(1).
#include <mdspan>
#include <array>
#include <type_traits>
#include "check.hpp"

using std::dynamic_extent;
template <std::size_t P, class E> using LP = typename std::layout_left_padded<P>::template mapping<E>;
template <std::size_t P, class E> using RP = typename std::layout_right_padded<P>::template mapping<E>;
template <class E> using L = std::layout_left::mapping<E>;
template <class E> using R = std::layout_right::mapping<E>;
template <class E> using S = std::layout_stride::mapping<E>;
using D2 = std::dextents<int, 2>;
using S32 = std::extents<int, 3, dynamic_extent>; // static extent(0)
using D1 = std::dextents<int, 1>;
using S1 = std::extents<int, 4>;
using E0 = std::extents<int>;

template <class To, class From>
constexpr bool implicit = std::is_convertible_v<From, To>;
template <class To, class From>
constexpr bool explicit_only = std::is_constructible_v<To, From> && !std::is_convertible_v<From, To>;

// layout_left from layout_left_padded.
static_assert(implicit<L<D2>, LP<4, D2>> && implicit<L<D2>, LP<dynamic_extent, S32>>);
static_assert(explicit_only<L<S32>, LP<dynamic_extent, D2>>);
static_assert(implicit<R<D2>, RP<4, D2>> && explicit_only<R<std::extents<int, dynamic_extent, 3>>, RP<4, D2>>);
static_assert(!std::is_constructible_v<L<D2>, RP<4, D2>> && !std::is_constructible_v<R<D2>, LP<4, D2>>);

// layout_left_padded from layout_left / layout_stride.
static_assert(implicit<LP<4, D2>, L<D2>> && explicit_only<LP<4, S32>, L<D2>>);
static_assert(explicit_only<LP<4, D2>, S<D2>> && explicit_only<LP<4, D1>, S<D1>>);
static_assert(implicit<LP<4, E0>, S<E0>> && implicit<RP<4, E0>, S<E0>>);
static_assert(implicit<RP<4, D2>, R<D2>> && explicit_only<RP<4, D2>, S<D2>>);

// layout_left_padded from layout_left_padded, rank 2.
static_assert(implicit<LP<dynamic_extent, D2>, LP<4, D2>>);           // dynamic from static padding
static_assert(implicit<LP<dynamic_extent, D2>, LP<4, S32>>);
static_assert(explicit_only<LP<4, D2>, LP<4, S32>>);                   // static padding_value
static_assert(explicit_only<LP<4, D2>, LP<dynamic_extent, D2>>);
static_assert(explicit_only<LP<dynamic_extent, D2>, LP<dynamic_extent, S32>>); // other's is dynamic
static_assert(explicit_only<LP<dynamic_extent, S32>, LP<4, D2>>);      // extents not convertible
static_assert(implicit<RP<dynamic_extent, D2>, RP<8, D2>> && explicit_only<RP<8, D2>, RP<8, std::extents<int, dynamic_extent, 3>>>);
// Rank 1: only the extents matter.
static_assert(implicit<LP<4, D1>, LP<8, S1>> && implicit<LP<4, D1>, LP<dynamic_extent, D1>>);
static_assert(explicit_only<LP<4, S1>, LP<4, D1>>);

// layout_left_padded from layout_right_padded / layout_right: rank 0 and 1 only.
static_assert(implicit<LP<4, D1>, RP<4, D1>> && implicit<LP<4, D1>, R<S1>> && explicit_only<LP<4, S1>, RP<2, D1>>);
static_assert(implicit<RP<4, D1>, LP<3, D1>> && implicit<RP<4, D1>, L<D1>>);
static_assert(implicit<LP<4, E0>, RP<4, E0>>);
static_assert(!std::is_constructible_v<LP<4, D2>, RP<4, D2>> && !std::is_constructible_v<LP<4, D2>, R<D2>>);
static_assert(!std::is_constructible_v<RP<4, D2>, LP<4, D2>>);

// layout_stride from padded mappings.
static_assert(implicit<S<D2>, LP<4, D2>> && implicit<S<D2>, RP<dynamic_extent, S32>>);
static_assert(explicit_only<S<S32>, LP<4, D2>>);

constexpr bool run() {
  // From layout_left: stride(1) is recomputed from the padding value.
  LP<4, D2> fromleft = L<D2>(D2(8, 3));
  if (fromleft.stride(1) != 8 || fromleft.extents() != D2(8, 3)) return false;
  // From layout_stride with stride(0) == 1 and a padded stride(1).
  LP<dynamic_extent, D2> fromstride(S<D2>(D2(3, 5), std::array<int, 2>{1, 7}));
  if (fromstride.stride(1) != 7 || fromstride(2, 4) != 2 + 28) return false;
  RP<dynamic_extent, D2> rfromstride(S<D2>(D2(5, 3), std::array<int, 2>{7, 1}));
  if (rfromstride.stride(0) != 7 || rfromstride(4, 2) != 28 + 2) return false;

  // Between padded mappings: the other's stride(1) is kept.
  LP<4, D2> src(D2(3, 5)); // stride(1) = 4
  LP<dynamic_extent, D2> dyn = src;
  if (dyn.stride(1) != 4 || !(dyn == src) || !(src == dyn)) return false;
  LP<4, D2> back(dyn);
  if (back.stride(1) != 4 || !(back == src)) return false;
  LP<dynamic_extent, D2> other(D2(3, 5), 6);
  if (other == src || other.stride(1) != 6) return false;
  if (LP<4, D2>(D2(3, 5)) == LP<4, D2>(D2(3, 6))) return false;
  // Rank 1 padded mappings compare their extents only.
  if (!(LP<4, D1>(D1(3)) == LP<dynamic_extent, D1>(D1(3), 5))) return false;

  // To layout_left (exhaustive: stride(1) == extent(0)), and to layout_stride.
  LP<4, D2> exh(D2(8, 2));
  L<D2> l = exh;
  if (!(l == L<D2>(D2(8, 2)))) return false;
  S<D2> st = src;
  if (st.strides() != std::array<int, 2>{1, 4} || !(st == src) || st.required_span_size() != src.required_span_size()) return false;
  RP<4, D2> rsrc(D2(5, 3)); // stride(0) = 4
  S<D2> rst = rsrc;
  if (rst.strides() != std::array<int, 2>{4, 1} || !(rst == rsrc)) return false;

  // Rank 1 left/right padded interconversion: the mapping is the identity.
  LP<4, D1> lp1 = RP<8, D1>(D1(5));
  if (lp1(4) != 4 || lp1.required_span_size() != 5) return false;
  // Rank 0.
  LP<4, E0> lp0 = S<E0>();
  if (lp0() != 0 || lp0.required_span_size() != 1) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
