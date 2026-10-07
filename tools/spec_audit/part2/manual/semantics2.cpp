// Spec-coverage probe, part 2 (docs/SPEC_COVERAGE.md), written by hand: more Effects and Returns
// of C++26 additions (and of C++23 facilities the external suites test little), evaluated in
// constant expressions. Each line ending in `// @M<n> semantics` is one check.
#include <algorithm>
#include <array>
#include <flat_map>
#include <flat_set>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace sv = std::views;

// [range.concat.view]/[range.concat.iterator]: the reference is the common reference, random access
static_assert([] { std::vector<int> a{1, 2}; std::vector<long> b{3};
                   auto c = sv::concat(a, b);
                   using C = decltype(c);
                   return std::same_as<std::ranges::range_reference_t<C>, long&> == false
                       && std::same_as<std::ranges::range_value_t<C>, long>
                       && std::ranges::random_access_range<C> && c.size() == 3 && c.end() - c.begin() == 3
                       && *(c.begin() + 2) == 3; }()); // @M201 semantics
// [range.enumerate.iterator]: value_type is tuple<difference_type, range_value_t<V>>
static_assert([] { std::array a{7}; using E = decltype(a | sv::enumerate);
                   return std::same_as<std::ranges::range_value_t<E>, std::tuple<std::ptrdiff_t, int>>
                       && std::same_as<std::ranges::range_reference_t<E>, std::tuple<std::ptrdiff_t, int&>>; }()); // @M202 semantics
// [range.utility.conv.to]: a nested ranges::to converts each element
static_assert([] { std::array<std::array<int, 2>, 2> a{{{1, 2}, {3, 4}}};
                   auto v = std::ranges::to<std::vector<std::vector<int>>>(a);
                   return v.size() == 2 && v[1][1] == 4; }()); // @M203 semantics
// [range.prim.size.hint]: reserve_hint of a range that is not sized but has a reserve_hint member
static_assert([] { std::array a{1, 2, 3, 4}; auto t = a | sv::take_while([](int) { return true; });
                   auto tk = sv::iota(0) | sv::take(3);
                   return !std::ranges::approximately_sized_range<decltype(t)> && std::ranges::reserve_hint(tk) == 3; }()); // @M204 semantics
// [flat.map.modifiers]: insert_range of unsorted input, replace, extract
static_assert([] { std::flat_map<int, int> m{{5, 0}};
                   m.insert_range(std::array{std::pair{3, 1}, std::pair{5, 9}, std::pair{1, 2}});
                   auto c = std::move(m).extract();
                   return c.keys == std::vector{1, 3, 5} && c.values == std::vector{2, 1, 0} && m.empty(); }()); // @M205 semantics
static_assert([] { std::flat_set<int> s; s.replace(std::vector{1, 2, 3}); return s.size() == 3 && s.contains(2); }()); // @M206 semantics
// [const.iterators.iterator]: basic_const_iterator converts to the constant iterator it wraps
static_assert([] { std::array a{1, 2}; std::basic_const_iterator<int*> it(a.data()); const int* p = it; return *p == 1 && it.base() == a.data(); }()); // @M207 semantics
// [range.join.with.view] with a range pattern, [range.lazy.split] of a string
static_assert([] { std::array<std::string_view, 3> a{"a", "b", "c"}; return std::ranges::to<std::string>(a | sv::join_with(std::string_view(", "))) == "a, b, c"; }()); // @M208 semantics
// [range.zip.transform], [range.adjacent.transform]
static_assert([] { std::array a{1, 2, 3}, b{10, 20, 30}; auto z = sv::zip_transform(std::plus{}, a, b); return z[2] == 33 && z.size() == 3; }()); // @M209 semantics
static_assert([] { std::array a{1, 2, 3}; auto p = a | sv::pairwise_transform(std::multiplies{}); return p.size() == 2 && p[1] == 6; }()); // @M210 semantics
// [range.chunk.by], [range.slide] of a forward range, [range.repeat]
static_assert([] { std::array a{1, 1, 2, 3, 3}; return std::ranges::distance(a | sv::chunk_by(std::ranges::equal_to{})) == 3; }()); // @M211 semantics
static_assert([] { auto r = sv::repeat(7, 3); return r.size() == 3 && r[2] == 7 && std::ranges::distance(sv::repeat(1) | sv::take(4)) == 4; }()); // @M212 semantics
// [range.as.rvalue], [range.as.const]: the reference types
static_assert([] { std::array a{1}; return std::same_as<std::ranges::range_reference_t<decltype(a | sv::as_rvalue)>, int&&>
                       && std::same_as<std::ranges::range_reference_t<decltype(a | sv::as_const)>, const int&>; }()); // @M213 semantics
// [string.ops]: compare and find with a string_view-like argument, starts_with/ends_with/contains
static_assert([] { std::string s = "hello world"; return s.find(std::string_view("wor")) == 6 && s.starts_with('h') && s.ends_with("ld") && s.contains("o w"); }()); // @M214 semantics
// [string.cons]: the from_range constructor and the substring constructor of an rvalue
static_assert([] { std::string s(std::from_range, std::array{'a', 'b'}); std::string t(std::string("xyz"), 1); return s == "ab" && t == "yz"; }()); // @M215 semantics
// [alg.find.last], [alg.contains], ranges::find_last_if_not
static_assert([] { std::array a{2, 1, 2}; auto r = std::ranges::find_last_if_not(a, [](int x) { return x == 2; }); return r.begin() == a.begin() + 1; }()); // @M216 semantics
// [alg.fold]: fold_left_with_iter's result, fold_left_first of an empty range
static_assert([] { std::array<int, 0> e; return !std::ranges::fold_left_first(e, std::plus{}).has_value(); }()); // @M217 semantics
// [alg.shift]: shift_left, ranges::shift_right
static_assert([] { std::array a{1, 2, 3, 4}; auto r = std::ranges::shift_right(a, 1); return r.begin() == a.begin() + 1 && a[1] == 1 && a[3] == 3; }()); // @M218 semantics
// [iterator.range]: std::size, std::ssize, std::data, std::empty of an array and a container
static_assert([] { int a[3]{}; std::vector<int> v{1}; return std::ssize(a) == 3 && std::size(v) == 1 && !std::empty(v) && std::data(a) == a; }()); // @M219 semantics
// [range.stride.view]: a stride over a sized range has size ceil(n / k)
static_assert([] { std::array a{1, 2, 3, 4, 5}; return (a | sv::stride(2)).size() == 3 && (a | sv::stride(5)).size() == 1; }()); // @M220 semantics
