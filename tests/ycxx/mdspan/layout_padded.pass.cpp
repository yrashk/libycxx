// [mdspan.layout.leftpad], [mdspan.layout.rightpad]: layout_left_padded<P>::mapping behaves
// like layout_left except that stride(1) is LEAST-MULTIPLE-AT-LEAST(P, extent(0)) (or a
// padding given at run time for P == dynamic_extent); stride(r) for r >= 2 is stride(1) times
// extent(1) ... extent(r-1); required_span_size() is m(extent(r) - 1 ...) + 1 (0 when empty);
// is_exhaustive() iff extent(0) == stride(1). layout_right_padded is the mirror image, padding
// stride(rank - 2) to a multiple of P at least extent(rank - 1). padding_value is P.
#include <mdspan>
#include <array>
#include <concepts>
#include <type_traits>
#include "check.hpp"

using std::dynamic_extent;
using E = std::extents<int, dynamic_extent, dynamic_extent, 2>;
using LP4 = std::layout_left_padded<4>::mapping<E>;
using RP4 = std::layout_right_padded<4>::mapping<std::extents<int, 2, dynamic_extent, dynamic_extent>>;
using LPD = std::layout_left_padded<dynamic_extent>::mapping<E>;

static_assert(LPD::padding_value == dynamic_extent);
static_assert(LP4::padding_value == 4 && RP4::padding_value == 4);
static_assert(std::is_same_v<LP4::layout_type, std::layout_left_padded<4>>);
static_assert(std::is_same_v<RP4::layout_type, std::layout_right_padded<4>>);
static_assert(LP4::is_always_unique() && LP4::is_always_strided() && !LP4::is_always_exhaustive());
static_assert(std::is_trivially_copyable_v<LP4> && std::regular<LP4> && std::regular<RP4>);
// Static first extent equal to the padded stride: always exhaustive.
static_assert(std::layout_left_padded<4>::mapping<std::extents<int, 8, dynamic_extent>>::is_always_exhaustive());
static_assert(!std::layout_left_padded<4>::mapping<std::extents<int, 6, dynamic_extent>>::is_always_exhaustive());
static_assert(std::layout_left_padded<4>::mapping<std::extents<int, dynamic_extent>>::is_always_exhaustive());  // rank 1

constexpr bool run() {
  LP4 l(E(3, 5));  // extents 3 x 5 x 2, stride(1) = 4
  if (l.stride(0) != 1 || l.stride(1) != 4 || l.stride(2) != 20) return false;
  if (l.strides() != std::array<int, 3>{1, 4, 20}) return false;
  if (l(2, 4, 1) != 2 + 16 + 20) return false;
  if (l.required_span_size() != 38 + 1) return false;
  if (l.is_exhaustive()) return false;
  LP4 l8(E(8, 5));  // 8 is already a multiple of 4
  if (l8.stride(1) != 8 || !l8.is_exhaustive() || l8.required_span_size() != 80) return false;
  LP4 l0(E(0, 5));  // LEAST-MULTIPLE-AT-LEAST(4, 0) == 0
  if (l0.stride(1) != 0 || l0.required_span_size() != 0) return false;

  LPD d(E(3, 5), 7);  // run-time padding: stride(1) = 7
  if (d.stride(1) != 7 || d.stride(2) != 35 || d(1, 1, 1) != 1 + 7 + 35) return false;
  LPD dd(E(3, 5));  // no padding given: stride(1) = extent(0)
  if (dd.stride(1) != 3 || !dd.is_exhaustive()) return false;
  if (!(dd == LPD(E(3, 5), 3))) return false;
  if (dd == d) return false;

  RP4 r(std::extents<int, 2, dynamic_extent, dynamic_extent>(5, 3));  // 2 x 5 x 3, stride(1) = 4
  if (r.stride(2) != 1 || r.stride(1) != 4 || r.stride(0) != 20) return false;
  if (r(1, 4, 2) != 20 + 16 + 2 || r.required_span_size() != 39) return false;
  if (r.is_exhaustive()) return false;

  // From layout_left: the padding is recomputed from the extents.
  std::layout_left::mapping<E> plain(E(4, 5));
  LP4 fromleft(plain);
  if (fromleft.stride(1) != 4 || !fromleft.is_exhaustive()) return false;
  // To layout_left (precondition: stride(1) == extent(0)).
  std::layout_left::mapping<E> back(fromleft);
  if (!(back == plain)) return false;
  // To layout_stride.
  std::layout_stride::mapping<E> st(l);
  if (st.stride(1) != 4 || st.stride(2) != 20 || !(st == l)) return false;
  // Rank 1: no padding stride; same as layout_left / layout_right.
  std::layout_right_padded<4>::mapping<std::extents<int, dynamic_extent>> r1(std::extents<int, dynamic_extent>(3));
  if (r1.stride(0) != 1 || r1.required_span_size() != 3 || !r1.is_exhaustive()) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
