// submdspan over every combination of slice kinds -- an index, full_extent, a pair {first,
// last}, range_slice{first, last, stride} and extent_slice{offset, extent, stride} -- in each
// dimension, with every valid run-time value of those slices, for rank-2 sources in
// layout_left, layout_right, layout_stride (non-contiguous strides), layout_left_padded and
// layout_right_padded, and rank-3 sources in layout_left and layout_right_padded.
// [mdspan.sub.sub]: the result has one dimension per non-collapsing slice; its extents are the
// sizes of the slices' index sets ([mdspan.sub.extents]); every element sub[i...] is the source
// element at the slices' selected indices ([mdspan.sub.map.common]: the mapping's offset is the
// source mapping of the slices' first indices). [mdspan.sub.helpers]/7: a range slice of span
// n and stride s has 1 + (n - 1) / s indices (none for n == 0, then stride 1 is used);
// [mdspan.sub.overview]/9: extent_slice may have stride 0 when its extent is 0 or 1. Checked
// by comparing element addresses with the source's, and the result's required_span_size()
// against the largest offset used.
#include <mdspan>
#include <array>
#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>
#include "check.hpp"

using std::dynamic_extent;

// One slice kind: the slice object plus the source indices it selects and whether it collapses
// the dimension (index slices).
struct Sel {
  std::vector<int> idx;
  bool collapses;
};

template <int Kind>
struct Kinds;
template <>
struct Kinds<0> {  // index
  static void each(int n, auto f) {
    for (int i = 0; i < n; ++i) f(i, Sel{{i}, true});
  }
};
template <>
struct Kinds<1> {  // full_extent
  static void each(int n, auto f) {
    Sel s{{}, false};
    for (int i = 0; i < n; ++i) s.idx.push_back(i);
    f(std::full_extent, s);
  }
};
template <>
struct Kinds<2> {  // pair
  static void each(int n, auto f) {
    for (int a = 0; a <= n; ++a)
      for (int b = a; b <= n; ++b) {
        Sel s{{}, false};
        for (int i = a; i < b; ++i) s.idx.push_back(i);
        f(std::pair<int, int>{a, b}, s);
      }
  }
};
template <>
struct Kinds<3> {  // range_slice
  static void each(int n, auto f) {
    for (int a = 0; a <= n; ++a)
      for (int b = a; b <= n; ++b)
        for (int st = 1; st <= 3; ++st) {
          Sel s{{}, false};
          for (int i = a; i < b; i += st) s.idx.push_back(i);
          f(std::range_slice<int, int, int>{a, b, st}, s);
        }
  }
};
template <>
struct Kinds<4> {  // extent_slice
  static void each(int n, auto f) {
    for (int o = 0; o <= n; ++o)
      for (int e = 0; e <= n; ++e)
        for (int st = 0; st <= 3; ++st) {
          if (e >= 2 && st == 0) continue;
          if (e > 0 && o + 1 + (e - 1) * st > n) continue;
          Sel s{{}, false};
          for (int i = 0; i < e; ++i) s.idx.push_back(o + i * st);
          f(std::extent_slice<int, int, int>{o, e, st}, s);
        }
  }
};

template <class Src, class Sub, std::size_t R>
void verify(const Src& src, const Sub& sub, const std::array<Sel, R>& sels) {
  std::size_t kept = 0;
  for (auto& s : sels) kept += !s.collapses;
  CHECK(static_cast<std::size_t>(Sub::rank()) == kept);
  {
    std::size_t r = 0;
    for (auto& s : sels)
      if (!s.collapses) CHECK(sub.extent(r++) == static_cast<typename Sub::index_type>(s.idx.size()));
  }
  // walk every index of sub
  std::size_t total = 1;
  for (auto& s : sels)
    if (!s.collapses) total *= s.idx.size();
  std::size_t max_offset = 0;
  for (std::size_t lin = 0; lin < total; ++lin) {
    std::array<typename Sub::index_type, Sub::rank()> si{};
    std::array<typename Src::index_type, R> srci{};
    std::size_t rest = lin, r = 0;
    for (std::size_t d = 0; d < R; ++d) {
      if (sels[d].collapses) {
        srci[d] = sels[d].idx[0];
      } else {
        std::size_t n = sels[d].idx.size();
        std::size_t k = rest % n;
        rest /= n;
        if constexpr (Sub::rank() > 0) si[r] = static_cast<typename Sub::index_type>(k);
        ++r;
        srci[d] = sels[d].idx[k];
      }
    }
    const auto* want = &src[srci];
    const auto* got = &sub[si];
    CHECK(got == want);
    std::size_t off = static_cast<std::size_t>(std::apply(sub.mapping(), si));
    if (off > max_offset) max_offset = off;
    CHECK(sub.data_handle() + off == got);
  }
  if (total > 0) CHECK(static_cast<std::size_t>(sub.mapping().required_span_size()) >= max_offset + 1);
  else CHECK(sub.size() == 0);
}

template <class Src, int K0, int K1>
void combo2(const Src& src) {
  Kinds<K0>::each(static_cast<int>(src.extent(0)), [&](auto s0, const Sel& a) {
    Kinds<K1>::each(static_cast<int>(src.extent(1)), [&](auto s1, const Sel& b) {
      auto sub = std::submdspan(src, s0, s1);
      verify(src, sub, std::array<Sel, 2>{a, b});
    });
  });
}
template <class Src, int K0, int K1, int K2>
void combo3(const Src& src) {
  Kinds<K0>::each(static_cast<int>(src.extent(0)), [&](auto s0, const Sel& a) {
    Kinds<K1>::each(static_cast<int>(src.extent(1)), [&](auto s1, const Sel& b) {
      Kinds<K2>::each(static_cast<int>(src.extent(2)), [&](auto s2, const Sel& c) {
        auto sub = std::submdspan(src, s0, s1, s2);
        verify(src, sub, std::array<Sel, 3>{a, b, c});
      });
    });
  });
}

template <class Src, int... K>
void all2(const Src& src, std::integer_sequence<int, K...>) {
  (combo2<Src, K / 5, K % 5>(src), ...);
}
template <class Src, int... K>
void all3(const Src& src, std::integer_sequence<int, K...>) {
  (combo3<Src, K / 25, (K / 5) % 5, K % 5>(src), ...);
}

int buffer[1000];

int main() {
  for (int i = 0; i < 1000; ++i) buffer[i] = i;
  using E2 = std::dextents<int, 2>;
  using E3 = std::extents<int, dynamic_extent, 3, dynamic_extent>;
  auto seq2 = std::make_integer_sequence<int, 25>{};
  auto seq3 = std::make_integer_sequence<int, 125>{};
  {
    std::mdspan<int, E2, std::layout_left> m(buffer, 4, 3);
    all2(m, seq2);
  }
  {
    std::mdspan<int, E2, std::layout_right> m(buffer, 3, 4);
    all2(m, seq2);
  }
  {
    std::layout_stride::mapping<E2> map(E2(3, 4), std::array<int, 2>{2, 9});
    std::mdspan<int, E2, std::layout_stride> m(buffer, map);
    all2(m, seq2);
  }
  {
    using L = std::layout_left_padded<dynamic_extent>;
    std::mdspan<int, E2, L> m(buffer, L::mapping<E2>(E2(3, 4), 5));
    CHECK(m.stride(1) == 5);
    all2(m, seq2);
  }
  {
    using L = std::layout_right_padded<4>;
    std::mdspan<int, E2, L> m(buffer, L::mapping<E2>(E2(4, 3)));
    CHECK(m.stride(0) == 4);
    all2(m, seq2);
  }
  {
    std::mdspan<int, E3, std::layout_left> m(buffer, 3, 2);
    all3(m, seq3);
  }
  {
    using L = std::layout_right_padded<dynamic_extent>;
    std::mdspan<int, E3, L> m(buffer, L::mapping<E3>(E3(2, 3), 4));
    CHECK(m.stride(1) == 4);
    all3(m, seq3);
  }
}
