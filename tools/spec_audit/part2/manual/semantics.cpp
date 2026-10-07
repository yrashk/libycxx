// Spec-coverage probe, part 2 (docs/SPEC_COVERAGE.md), written by hand: Effects and Returns of
// the C++26 additions to [containers], [ranges], [algorithms] and [strings], evaluated in constant
// expressions. Each line ending in `// @M<n> semantics` is one check; the comment before a group
// names the subclauses.
#include <algorithm>
#include <array>
#include <flat_map>
#include <inplace_vector>
#include <mdspan>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sv = std::views;
template <class R, class... T>
constexpr bool eq(R&& r, T... v) {
  std::array<std::common_type_t<T...>, sizeof...(T)> want{v...};
  return std::ranges::equal(r, want);
}

// [range.iota.view] views::indices, [range.concat], [range.cache.latest], [range.as.input]
static_assert(eq(sv::indices(3), 0, 1, 2)); // @M101 semantics
static_assert(std::same_as<std::ranges::range_value_t<decltype(sv::indices(3u))>, unsigned>); // @M102 semantics
static_assert([] { std::vector a{1, 2}; std::array b{3}; return eq(sv::concat(a, b), 1, 2, 3); }()); // @M103 semantics
static_assert([] { std::vector a{1, 2, 3}; auto r = a | sv::transform([](int x) { return x * 2; }) | sv::cache_latest; return eq(r, 2, 4, 6); }()); // @M104 semantics
static_assert([] { std::vector a{1, 2, 3}; auto r = a | sv::as_input; return !std::ranges::forward_range<decltype(r)> && std::ranges::sized_range<decltype(r)> && eq(r, 1, 2, 3); }()); // @M105 semantics
// [range.prim.size.hint] ranges::reserve_hint, [range.utility.conv.to] ranges::to
static_assert([] { std::vector a{1, 2, 3}; return std::ranges::reserve_hint(a) == 3; }()); // @M106 semantics
static_assert([] { auto v = sv::iota(0, 4) | std::ranges::to<std::vector>(); return eq(v, 0, 1, 2, 3); }()); // @M107 semantics
static_assert([] { auto s = sv::iota('a', 'd') | std::ranges::to<std::string>(); return s == "abc"; }()); // @M108 semantics
// [range.enumerate], [range.zip], [range.adjacent], [range.chunk], [range.slide], [range.stride], [range.join.with]
static_assert([] { std::array a{5, 6}; auto [i, x] = *std::ranges::next((a | sv::enumerate).begin()); return i == 1 && x == 6; }()); // @M109 semantics
static_assert([] { std::array a{1, 2, 3}; return std::ranges::distance(sv::pairwise(a)) == 2 && std::ranges::distance(a | sv::chunk(2)) == 2 && std::ranges::distance(a | sv::slide(2)) == 2; }()); // @M110 semantics
static_assert([] { std::array a{1, 2, 3, 4, 5}; return eq(a | sv::stride(2), 1, 3, 5); }()); // @M111 semantics
static_assert([] { std::array<std::string_view, 2> a{"ab", "c"}; return std::ranges::to<std::string>(a | sv::join_with('-')) == "ab-c"; }()); // @M112 semantics
static_assert([] { std::string_view s = "a,b"; return std::ranges::distance(s | sv::split(',')) == 2 && std::ranges::distance(s | sv::lazy_split(',')) == 2; }()); // @M113 semantics
static_assert([] { std::array a{1, 2}, b{3, 4}; return std::ranges::distance(sv::cartesian_product(a, b)) == 4; }()); // @M114 semantics
// [alg.find.last], [alg.contains], [alg.fold], [alg.starts.with]
static_assert([] { std::array a{1, 2, 1, 3}; auto r = std::ranges::find_last(a, 1); return r.begin() == a.begin() + 2 && r.end() == a.end(); }()); // @M115 semantics
static_assert([] { std::array a{1, 2, 3}; std::array b{2, 3}; return std::ranges::contains_subrange(a, b) && std::ranges::ends_with(a, b) && std::ranges::starts_with(a, std::array{1}); }()); // @M116 semantics
static_assert([] { std::array a{1, 2, 3}; auto r = std::ranges::fold_left_first_with_iter(a, std::plus{}); return r.in == a.end() && r.value == 6; }()); // @M117 semantics
static_assert([] { std::array a{1, 2, 3}; return std::ranges::fold_right(a, 0, std::minus{}) == 2; }()); // @M118 semantics
// [span.cons] the initializer_list constructor (P2447), [span.elem] at
static_assert([] { auto f = [](std::span<const int> s) { return s.size() == 3 && s.at(1) == 2; }; return f({1, 2, 3}); }()); // @M119 semantics
// [inplace.vector.modifiers] try_ and unchecked_ forms
static_assert([] { std::inplace_vector<int, 2> v; v.try_emplace_back(1); v.try_push_back(2); auto r = v.try_push_back(3); return v.size() == 2 && !r && v.back() == 2; }()); // @M120 semantics
// [flat.map.modifiers] insert_range with sorted_unique, [flat.map.access]
static_assert([] { std::flat_map<int, int> m; m.insert_range(std::sorted_unique, std::array{std::pair{1, 2}, std::pair{3, 4}}); return m.size() == 2 && m.keys()[1] == 3; }()); // @M121 semantics
// [mdspan.sub] submdspan with full_extent_t, an index and extent_slice / range_slice
static_assert([] { int a[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; std::mdspan m(a, 3, 4);
                   auto row = std::submdspan(m, 1, std::full_extent);
                   return row.rank() == 1 && row.extent(0) == 4 && row[2] == 6; }()); // @M122 semantics
static_assert([] { int a[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; std::mdspan m(a, 3, 4);
                   auto s = std::submdspan(m, std::full_extent, std::extent_slice{1, 2, 2});
                   return s.extent(1) == 2 && s[0, 0] == 1 && s[0, 1] == 3 && s[2, 1] == 11; }()); // @M123 semantics
static_assert([] { int a[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; std::mdspan m(a, 3, 4);
                   auto s = std::submdspan(m, std::range_slice{1, 3}, 0);
                   return s.extent(0) == 2 && s[0] == 4 && s[1] == 8; }()); // @M124 semantics
// [string.ops] subview ([string.view.ops]), [string.conversions]
static_assert([] { std::string s = "abcdef"; return s.subview(2) == "cdef" && s.subview(1, 2) == "bc"; }()); // @M125 semantics
static_assert([] { return std::to_string(-2147483647 - 1) == "-2147483648"; }()); // @M126 semantics
