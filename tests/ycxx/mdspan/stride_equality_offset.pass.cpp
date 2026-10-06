// [mdspan.layout.stride.obs]/7-9: layout_stride::mapping == another strided mapping (a
// layout-mapping-alike type, [mdspan.layout.stride.expo]/4, of the same rank and always
// strided) is true iff the extents are equal, OFFSET(y) == 0 ([mdspan.layout.stride.expo]/2:
// y(0, ..., 0) for a non-empty index space) and every stride is equal: a user mapping that adds
// a constant offset compares unequal even with the same extents and strides, and one without
// the offset compares equal. Mappings that are not always strided, or of another rank, do not
// take part (the operator is constrained away).
#include <mdspan>
#include <array>
#include <cstddef>
#include "check.hpp"

using E = std::dextents<int, 2>;

// A strided user mapping: offset + i * s0 + j * s1 (meeting the layout mapping requirements,
// [mdspan.layout.reqmts], as /8 requires).
template <bool AlwaysStrided>
struct shifted;
template <bool AlwaysStrided>
struct shifted_layout {
  template <class Ext>
  using mapping = shifted<AlwaysStrided>;
};
template <bool AlwaysStrided>
struct shifted {
  using extents_type = E;
  using index_type = int;
  using size_type = unsigned;
  using rank_type = std::size_t;
  using layout_type = shifted_layout<AlwaysStrided>;
  E ext;
  std::array<int, 2> s;
  int offset;
  constexpr const E& extents() const noexcept { return ext; }
  constexpr int operator()(int i, int j) const noexcept { return offset + i * s[0] + j * s[1]; }
  constexpr int required_span_size() const noexcept {
    return offset + (ext.extent(0) - 1) * s[0] + (ext.extent(1) - 1) * s[1] + 1;
  }
  constexpr int stride(std::size_t r) const noexcept { return s[r]; }
  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept { return false; }
  static constexpr bool is_always_strided() noexcept { return AlwaysStrided; }
  static constexpr bool is_unique() noexcept { return true; }
  constexpr bool is_exhaustive() const noexcept { return false; }
  static constexpr bool is_strided() noexcept { return AlwaysStrided; }
  friend constexpr bool operator==(const shifted&, const shifted&) = default;
};

using S = std::layout_stride::mapping<E>;
template <class A, class B>
concept eq_ok = requires(A a, B b) { a == b; };
static_assert(eq_ok<S, shifted<true>> && !eq_ok<S, shifted<false>>);
static_assert(!eq_ok<S, std::layout_right::mapping<std::dextents<int, 3>>>);

constexpr bool run() {
  S m(E(3, 4), std::array<int, 2>{4, 1});
  shifted<true> same{E(3, 4), {4, 1}, 0};
  shifted<true> moved{E(3, 4), {4, 1}, 5};
  shifted<true> other_strides{E(3, 4), {1, 3}, 0};
  shifted<true> other_extents{E(3, 5), {5, 1}, 0};
  if (!(m == same) || !(same == m)) return false;
  if (m == moved || moved == m) return false; // OFFSET(moved) == 5
  if (m == other_strides || m == other_extents) return false;
  // The standard strided layouts compare by the same rule.
  if (!(m == std::layout_right::mapping<E>(E(3, 4)))) return false;
  if (m == std::layout_left::mapping<E>(E(3, 4))) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
