// [mdspan.layout.stride]: layout_stride::mapping with user strides. Default construction uses
// layout_right's strides; mapping(e, s) for span/array strides; operator() is
// sum(i_r * stride(r)); required_span_size() is 1 + sum((extent(r) - 1) * stride(r)) (0 for an
// empty index space, 1 for rank 0); always unique and strided, always exhaustive only for rank 0
// or a static zero extent; is_exhaustive() is true iff some permutation of the strides is
// packed. Construction from any strided, unique layout mapping (implicit from left, right,
// padded and stride mappings with convertible extents); operator== compares extents, strides
// and offset.
#include <mdspan>
#include <array>
#include <span>
#include <type_traits>
#include "check.hpp"

using std::dynamic_extent;
using E = std::extents<int, 3, dynamic_extent>;
using S = std::layout_stride::mapping<E>;

static_assert(std::is_same_v<S::layout_type, std::layout_stride>);
static_assert(S::is_always_unique() && S::is_always_strided() && !S::is_always_exhaustive());
static_assert(std::layout_stride::mapping<std::extents<int>>::is_always_exhaustive());
static_assert(std::layout_stride::mapping<std::extents<int, 3, 0>>::is_always_exhaustive());
static_assert(std::is_trivially_copyable_v<S>);
static_assert(std::is_convertible_v<std::layout_right::mapping<E>, S>);
static_assert(std::is_convertible_v<std::layout_left::mapping<std::extents<int, 3, 4>>, S>);
static_assert(!std::is_convertible_v<std::layout_stride::mapping<std::dextents<int, 2>>, S>);
static_assert(std::is_constructible_v<S, std::layout_stride::mapping<std::dextents<int, 2>>>);
static_assert(std::is_same_v<decltype(S().strides()), std::array<int, 2>>);

static_assert(!std::is_constructible_v<S, E>);  // no mapping(const extents_type&)

constexpr bool run2() {
  S def;  // layout_right strides for E() = (3, 0)
  if (def.stride(0) != 0 || def.stride(1) != 1) return false;
  std::array<int, 2> st{1, 5};
  S m(E(4), st);
  if (m.stride(0) != 1 || m.stride(1) != 5 || m.strides() != st) return false;
  if (m(2, 3) != 2 + 15) return false;
  if (m.required_span_size() != 1 + 2 * 1 + 3 * 5) return false;  // 18
  if (m.is_exhaustive()) return false;                           // 3 * 1 != 5
  long raw[2] = {4, 1};
  S packed(E(4), std::span<long, 2>(raw));  // row-major packed
  if (!packed.is_exhaustive() || packed(2, 3) != 11 || packed.required_span_size() != 12) return false;
  S col(E(4), std::array<int, 2>{1, 3});  // column-major packed
  if (!col.is_exhaustive()) return false;
  // Equality with other strided mappings.
  if (!(packed == std::layout_right::mapping<E>(E(4)))) return false;
  if (!(col == std::layout_left::mapping<E>(E(4)))) return false;
  if (packed == col) return false;
  S fromr = std::layout_right::mapping<E>(E(4));
  if (fromr.stride(0) != 4 || fromr.stride(1) != 1) return false;
  // Empty and rank-0.
  S empty(E(0), std::array<int, 2>{1, 3});
  if (empty.required_span_size() != 0 || !empty.is_exhaustive()) return false;
  std::layout_stride::mapping<std::extents<int>> r0;
  if (r0.required_span_size() != 1 || r0() != 0) return false;
  return true;
}
static_assert(run2());

int main() {
  CHECK(run2());
  return 0;
}
