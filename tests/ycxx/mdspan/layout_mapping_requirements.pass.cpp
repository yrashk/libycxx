// [mdspan.layout.reqmts]/1-28, checked as an oracle over every multidimensional index of each
// standard mapping (layout_left, layout_right, layout_stride, layout_left_padded and
// layout_right_padded) for several extents, in constant evaluation:
//   /1: copyable, equality_comparable, nothrow move construction/assignment and swap;
//   /2-5: extents_type is an extents, index_type and rank_type are the extents', layout_type's
//     mapping<extents_type> is M; /6: extents() returns const extents_type&;
//   /7-10: m(i...) is an index_type, non-negative and below numeric_limits<index_type>::max(),
//     the same for indices converted to index_type first;
//   /11-12: required_span_size() is 0 for an empty index space, else 1 + max m(i...);
//   /13-14: is_unique() only if distinct indices map to distinct offsets;
//   /15-16: is_exhaustive() only if every offset in [0, required_span_size()) is hit;
//   /17-21: is_strided() only if m(i + d_r) - m(i) is a constant s_r, and stride(r) is s_r;
//   /23-28: is_always_X() is a constant expression and implies is_X().
#include <mdspan>
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>
#include "check.hpp"

using std::dynamic_extent;

template <class M>
consteval bool static_requirements() {
  using E = typename M::extents_type;
  static_assert(std::copyable<M> && std::equality_comparable<M>);
  static_assert(std::is_nothrow_move_constructible_v<M> && std::is_nothrow_move_assignable_v<M>);
  static_assert(std::is_nothrow_swappable_v<M>);
  static_assert(std::is_same_v<typename M::index_type, typename E::index_type>);
  static_assert(std::is_same_v<typename M::rank_type, typename E::rank_type>);
  static_assert(std::is_same_v<typename M::layout_type::template mapping<E>, M>);
  static_assert(std::is_same_v<decltype(std::declval<const M&>().extents()), const E&>);
  static_assert(std::is_same_v<decltype(std::declval<const M&>().required_span_size()), typename M::index_type>);
  static_assert(std::is_same_v<decltype(std::declval<const M&>().is_unique()), bool>);
  static_assert(std::is_same_v<decltype(std::declval<const M&>().is_exhaustive()), bool>);
  static_assert(std::is_same_v<decltype(std::declval<const M&>().is_strided()), bool>);
  constexpr bool u = M::is_always_unique(), e = M::is_always_exhaustive(), s = M::is_always_strided();
  return u || e || s || true;
}

// Calls f(idx) for every multidimensional index of e, idx a std::array<index_type, rank>.
template <class E, class F, std::size_t R = 0>
constexpr void for_each_index(const E& e, F& f, std::array<typename E::index_type, E::rank()>& idx) {
  if constexpr (R == E::rank()) {
    f(idx);
  } else {
    for (typename E::index_type i = 0; i < e.extent(R); ++i) {
      idx[R] = i;
      for_each_index<E, F, R + 1>(e, f, idx);
    }
  }
}

template <class M>
constexpr bool check_mapping(const M& m) {
  using E = typename M::extents_type;
  using I = typename M::index_type;
  constexpr std::size_t R = E::rank();
  const E& e = m.extents();
  M copy = m;
  if (!(copy == m)) return false;

  constexpr int cap = 256;
  bool hit[cap] = {};
  I offsets[cap] = {};
  int count = 0;
  long long maxoff = -1;
  bool ok = true;
  auto visit = [&](const std::array<I, R>& idx) {
    I o = [&]<std::size_t... P>(std::index_sequence<P...>) { return m(idx[P]...); }(std::make_index_sequence<R>());
    // /9: indices of another integer type give the same offset.
    long long o2 = [&]<std::size_t... P>(std::index_sequence<P...>) {
      return m(static_cast<short>(idx[P])...);
    }(std::make_index_sequence<R>());
    if (o != o2 || o < 0 || o >= std::numeric_limits<I>::max() || o >= cap) ok = false;
    else {
      if (m.is_unique() && hit[o]) ok = false;
      hit[o] = true;
    }
    if (count < cap) offsets[count++] = o;
    if (static_cast<long long>(o) > maxoff) maxoff = o;
    // /17-21: one step in each dimension adds stride(r).
    if constexpr (R > 0) {
      if (m.is_strided()) {
        for (std::size_t r = 0; r < R; ++r) {
          if (idx[r] + 1 < e.extent(r)) {
            std::array<I, R> next = idx;
            ++next[r];
            I on = [&]<std::size_t... P>(std::index_sequence<P...>) { return m(next[P]...); }(std::make_index_sequence<R>());
            if (on - o != m.stride(r)) ok = false;
          }
        }
      }
    }
  };
  std::array<I, R> idx{};
  for_each_index(e, visit, idx);
  if (!ok) return false;

  // /11-12
  I size = 1;
  for (std::size_t r = 0; r < R; ++r) size *= e.extent(r);
  if (size == 0) {
    if (m.required_span_size() != 0 || count != 0) return false;
  } else if (static_cast<long long>(m.required_span_size()) != maxoff + 1) {
    return false;
  }
  // /15-16
  if (m.is_exhaustive())
    for (I k = 0; k < m.required_span_size(); ++k)
      if (k >= cap || !hit[k]) return false;
  // /23-28
  if (M::is_always_unique() && !m.is_unique()) return false;
  if (M::is_always_exhaustive() && !m.is_exhaustive()) return false;
  if (M::is_always_strided() && !m.is_strided()) return false;
  return true;
}

template <class Layout, class E>
constexpr bool check_layout(const E& e) {
  static_assert(static_requirements<typename Layout::template mapping<E>>());
  return check_mapping(typename Layout::template mapping<E>(e));
}

template <class E>
constexpr bool all_layouts(const E& e) {
  return check_layout<std::layout_left>(e) && check_layout<std::layout_right>(e) &&
         check_layout<std::layout_left_padded<4>>(e) && check_layout<std::layout_right_padded<4>>(e) &&
         check_layout<std::layout_left_padded<dynamic_extent>>(e) &&
         check_layout<std::layout_right_padded<1>>(e) && check_layout<std::layout_left_padded<3>>(e);
}

constexpr bool run() {
  using E3 = std::extents<int, dynamic_extent, 3, dynamic_extent>;
  using E2 = std::dextents<long, 2>;
  if (!all_layouts(std::extents<int>())) return false;
  if (!all_layouts(std::extents<unsigned, 5>())) return false;
  if (!all_layouts(E2(3, 5)) || !all_layouts(E2(5, 3)) || !all_layouts(E2(0, 4)) || !all_layouts(E2(4, 0))) return false;
  if (!all_layouts(E3(2, 4)) || !all_layouts(E3(3, 2)) || !all_layouts(E3(1, 1))) return false;
  if (!all_layouts(std::extents<short, 2, 2, 2>())) return false;

  // layout_stride: packed, padded, permuted, overlapping-free gaps, and a zero extent.
  using S2 = std::layout_stride::mapping<E2>;
  static_assert(static_requirements<S2>());
  if (!check_mapping(S2(E2(3, 4), std::array<long, 2>{4, 1}))) return false;
  if (!check_mapping(S2(E2(3, 4), std::array<long, 2>{1, 3}))) return false;
  if (!check_mapping(S2(E2(3, 4), std::array<long, 2>{1, 5}))) return false;
  if (!check_mapping(S2(E2(3, 4), std::array<long, 2>{13, 3}))) return false;
  if (!check_mapping(S2(E2(0, 4), std::array<long, 2>{1, 7}))) return false;
  using S3 = std::layout_stride::mapping<E3>;
  if (!check_mapping(S3(E3(2, 2), std::array<int, 3>{3, 6, 1}))) return false;
  if (!check_mapping(S3(E3(2, 2), std::array<int, 3>{1, 2, 6}))) return false;
  if (!check_mapping(std::layout_stride::mapping<std::extents<int>>())) return false;

  // Padded mappings with run-time padding values.
  using LPD = std::layout_left_padded<dynamic_extent>::mapping<E3>;
  using RPD = std::layout_right_padded<dynamic_extent>::mapping<E3>;
  if (!check_mapping(LPD(E3(3, 2), 5)) || !check_mapping(RPD(E3(3, 2), 5))) return false;
  if (!check_mapping(LPD(E3(2, 2), 2)) || !check_mapping(RPD(E3(2, 4), 3))) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
