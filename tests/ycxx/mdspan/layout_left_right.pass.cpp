// [mdspan.layout.left], [mdspan.layout.right]: layout_left::mapping maps i... to
// sum(i_r * stride(r)) with stride(r) = product of extent(k) for k < r (leftmost extent has
// stride 1); layout_right uses the product of extent(k) for k > r. Both are always unique,
// exhaustive and strided, required_span_size() is the product of the extents, mappings
// compare equal iff their extents do, and they are trivially copyable and regular.
// Conversions: from the same layout with constructible extents; between left and right only
// for rank <= 1; from layout_stride (explicit unless rank 0).
// COUNTERPART: libcxx:containers/views/mdspan/layout_(left|right)/ctor.layout_stride.pass.cpp
// COUNTERPART: libcxx:containers/views/mdspan/layout_(left|right)/(ctor.default|ctor.extents|index_operator|required_span_size|stride).pass.cpp
#include <mdspan>
#include <array>
#include <concepts>
#include <type_traits>
#include "check.hpp"

using std::dynamic_extent;
using E = std::extents<int, 2, dynamic_extent, 4>;
using L = std::layout_left::mapping<E>;
using R = std::layout_right::mapping<E>;

static_assert(std::is_same_v<L::layout_type, std::layout_left> && std::is_same_v<R::layout_type, std::layout_right>);
static_assert(std::is_same_v<L::extents_type, E> && std::is_same_v<L::index_type, int>);
static_assert(std::is_same_v<R::size_type, unsigned> && std::is_same_v<R::rank_type, std::size_t>);
static_assert(std::is_trivially_copyable_v<L> && std::regular<L> && std::is_trivially_copyable_v<R> && std::regular<R>);
static_assert(L::is_always_unique() && L::is_always_exhaustive() && L::is_always_strided());
static_assert(R::is_always_unique() && R::is_always_exhaustive() && R::is_always_strided());
static_assert(L::is_unique() && L::is_exhaustive() && L::is_strided());
static_assert(std::is_nothrow_move_constructible_v<L> && std::is_nothrow_swappable_v<R>);

// Conversions.
using L1 = std::layout_left::mapping<std::extents<int, dynamic_extent>>;
using R1 = std::layout_right::mapping<std::extents<int, 5>>;
static_assert(std::is_constructible_v<L1, R1> && std::is_convertible_v<R1, L1>);
static_assert(!std::is_constructible_v<L, R>);  // rank 3
static_assert(!std::is_convertible_v<L1, R1> && std::is_constructible_v<R1, L1>);  // dynamic -> static is explicit
static_assert(std::is_constructible_v<L, std::layout_stride::mapping<E>>);
static_assert(!std::is_convertible_v<std::layout_stride::mapping<E>, L>);
static_assert(std::is_convertible_v<std::layout_stride::mapping<std::extents<int>>, std::layout_left::mapping<std::extents<int>>>);
static_assert(std::is_convertible_v<std::layout_left::mapping<std::extents<int, 2, 3, 4>>, L>);

constexpr bool run() {
  L l(E(3));
  R r(E(3));
  if (l.extents().extent(1) != 3 || l.required_span_size() != 24 || r.required_span_size() != 24) return false;
  if (l.stride(0) != 1 || l.stride(1) != 2 || l.stride(2) != 6) return false;
  if (r.stride(0) != 12 || r.stride(1) != 4 || r.stride(2) != 1) return false;
  if (l(1, 2, 3) != 1 + 2 * 2 + 3 * 6) return false;
  if (r(1, 2, 3) != 1 * 12 + 2 * 4 + 3) return false;
  if (l(0, 0, 0) != 0 || r(1, 2, 3) != 23 || l(1, 2, 3) != 23) return false;
  if (l(short(1), 2L, 3u) != 23) return false;  // mixed index types
  // Every index maps to a distinct offset in [0, 24).
  bool seen[24] = {};
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 4; ++k) {
        int o = r(i, j, k);
        if (o < 0 || o >= 24 || seen[o]) return false;
        seen[o] = true;
        if (l(i, j, k) != i + 2 * j + 6 * k) return false;
      }
  if (!(l == L(E(3))) || l == L(E(4))) return false;
  if (!(l == std::layout_left::mapping<std::extents<long, 2, 3, 4>>())) return false;
  // Empty index space.
  L z(E(0));
  if (z.required_span_size() != 0) return false;
  // Rank 0: one element at offset 0.
  std::layout_right::mapping<std::extents<int>> m0;
  if (m0() != 0 || m0.required_span_size() != 1) return false;
  // Rank-1 conversions between left and right, and from layout_stride.
  R1 r1;
  L1 l1 = r1;
  if (l1.extents().extent(0) != 5 || l1(4) != 4) return false;
  std::layout_stride::mapping<E> st(E(3), std::array<int, 3>{1, 2, 6});
  L fromst(st);
  if (!(fromst == l)) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
