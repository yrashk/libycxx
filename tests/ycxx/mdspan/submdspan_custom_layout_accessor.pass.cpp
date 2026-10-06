// [mdspan.sub.sub]: submdspan with a user layout and a user accessor.
// /2-/3: the slices are canonicalized (canonical_slices) and passed to submdspan_mapping, found
// by argument-dependent lookup; [mdspan.sub.helpers]/12 and [mdspan.sub.overview]/4: an integral
// slice arrives as index_type, a constant_wrapper as constant_wrapper<index_type(v)>, a pair or a
// range_slice as an extent_slice, full_extent as full_extent_t.
// /4.2: submdspan is constrained on the mapping modelling sliceable-mapping
// ([mdspan.sub.map.sliceable]/6: submdspan_mapping(lm, full_extent...) is well-formed and returns
// a submdspan_mapping_result).
// /7: the result is mdspan(src.accessor().offset(src.data_handle(), sub.offset), sub.mapping,
// AccessorPolicy::offset_policy(src.accessor())).
#include <mdspan>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

using std::full_extent;

template <class T, class IT>
concept cw_of = requires {
  requires std::is_same_v<std::remove_cvref_t<decltype(T::value)>, IT>;
  requires std::is_same_v<T, std::constant_wrapper<T::value>>;
  requires T::value >= 0;
};
template <class T, class IT>
concept canonical_index = std::is_same_v<T, IT> || cw_of<T, IT>;
template <class T, class IT>
constexpr bool is_canonical = canonical_index<T, IT> || std::is_same_v<T, std::full_extent_t>;
template <class IT, class O, class E, class S>
constexpr bool is_canonical<std::extent_slice<O, E, S>, IT> =
    canonical_index<O, IT> && canonical_index<E, IT> && canonical_index<S, IT>;

int custom_calls = 0;
int offset_calls = 0;

// layout_left under another name, sliced by delegating to layout_left's submdspan_mapping
struct my_left {
  template <class E>
  struct mapping : std::layout_left::mapping<E> {
    using base = std::layout_left::mapping<E>;
    using layout_type = my_left;
    using base::base;
    template <class... S>
    friend constexpr auto submdspan_mapping(const mapping& m, S... s) {
      static_assert((is_canonical<S, typename E::index_type> && ...), "slices are not canonical");
      ++custom_calls;
      return submdspan_mapping(static_cast<const base&>(m), s...);
    }
  };
};

// a layout without submdspan_mapping (not derived from a standard mapping, whose hidden friend
// submdspan_mapping argument-dependent lookup would find)
struct unsliceable {
  template <class E>
  struct mapping {
    using extents_type = E;
    using index_type = E::index_type;
    using size_type = E::size_type;
    using rank_type = E::rank_type;
    using layout_type = unsliceable;
    std::layout_right::mapping<E> m;
    constexpr const E& extents() const noexcept { return m.extents(); }
    constexpr index_type required_span_size() const noexcept { return m.required_span_size(); }
    template <class... I>
    constexpr index_type operator()(I... i) const noexcept { return m(i...); }
    static constexpr bool is_always_unique() noexcept { return true; }
    static constexpr bool is_always_exhaustive() noexcept { return true; }
    static constexpr bool is_always_strided() noexcept { return true; }
    static constexpr bool is_unique() noexcept { return true; }
    static constexpr bool is_exhaustive() noexcept { return true; }
    static constexpr bool is_strided() noexcept { return true; }
    constexpr index_type stride(rank_type r) const noexcept { return m.stride(r); }
    friend constexpr bool operator==(const mapping&, const mapping&) = default;
  };
};

template <class T>
struct counting_accessor {
  using element_type = T;
  using reference = T&;
  using data_handle_type = T*;
  using offset_policy = std::default_accessor<T>;
  constexpr reference access(T* p, std::size_t i) const noexcept { return p[i]; }
  constexpr T* offset(T* p, std::size_t i) const noexcept {
    ++offset_calls;
    return p + i;
  }
  constexpr operator std::default_accessor<T>() const noexcept { return {}; }
};

template <class M, class... S>
concept can_submdspan = requires(M m, S... s) { std::submdspan(m, s...); };

int main() {
  long data[60];
  for (int i = 0; i < 60; ++i) data[i] = i;
  using E = std::extents<long, 4, 3, 5>;
  std::mdspan<long, E, my_left, counting_accessor<long>> m(data);
  CHECK(m[1, 2, 3] == 1 + 4 * 2 + 12 * 3);

  // int, pair<int, int>, constant_wrapper<int>, range_slice: all canonicalized to long
  auto s = std::submdspan(m, 2, std::pair{1, 3}, std::range_slice{std::cw<1>, std::cw<5>, std::cw<2>});
  CHECK(custom_calls == 1 && offset_calls == 1);
  static_assert(std::is_same_v<decltype(s)::accessor_type, std::default_accessor<long>>);
  static_assert(decltype(s)::rank() == 2);
  CHECK(s.extent(0) == 2 && s.extent(1) == 2);
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j) CHECK(&s[i, j] == &m[2, 1 + i, 1 + 2 * j]);

  auto t = std::submdspan(m, std::cw<3>, full_extent, std::extent_slice{1, 2, 1});
  CHECK(custom_calls == 2 && offset_calls == 2);
  CHECK(t.extent(0) == 3 && t.extent(1) == 2 && &t[2, 1] == &m[3, 2, 2]);
  auto u = std::submdspan(m, full_extent, full_extent, full_extent);
  CHECK(custom_calls == 3 && u.data_handle() == data && &u[3, 2, 4] == &m[3, 2, 4]);

  // a collapsing slice at the last index: the offset is that element's
  auto v = std::submdspan(m, 3, 2, 4);
  static_assert(decltype(v)::rank() == 0);
  CHECK(&v[] == &m[3, 2, 4]);

  // /4.2: no submdspan_mapping, no submdspan
  using U = std::mdspan<long, E, unsliceable>;
  static_assert(!can_submdspan<U, int, int, int>);
  static_assert(can_submdspan<decltype(m), int, int, int>);
  // /4.1: one slice per extent
  static_assert(!can_submdspan<decltype(m), int, int>);
  return 0;
}
