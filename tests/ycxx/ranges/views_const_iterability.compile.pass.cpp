// Const-iterability of the range adaptors, from the constraints on their begin() const members:
// [range.filter.view] (const begin only for input-only const V), [range.drop.while.view],
// [range.chunk.by.view], [range.split.view], [range.cache.latest.view] (no begin() const);
// [range.lazy.split.view]: begin() const requires forward_range<V> && forward_range<const V> &&
// forward_range<const Pattern>; [range.slide.view]: begin() const requires
// slide-caches-nothing<const V> (random_access_range && sized_range); [range.stride.view],
// [range.as.rvalue.view], [range.as.input.view]: requires range<const V>; [range.enumerate.view]:
// requires range-with-movable-references<const V>; [range.cartesian.view], [range.concat.view]:
// every const view is a range; [range.join.with.view]: forward_range<const V>, forward_range<const
// Pattern>, is_reference_v<range_reference_t<const V>>; [range.transform.view],
// [range.zip.transform.view], [range.adjacent.transform.view]: regular_invocable<const F&, ...>
// (a mutable lambda makes them non-const-iterable).
#include <ranges>
#include <vector>
#include <list>

namespace rg = std::ranges;
namespace vw = std::views;

using V = rg::ref_view<std::vector<int>>;
using L = rg::ref_view<std::list<int>>;
inline constexpr auto pred = [](int x) { return x > 0; };
inline constexpr auto eq = [](int a, int b) { return a == b; };
inline constexpr auto plus = [](int a, int b) { return a + b; };
inline auto mut = [n = 0](int a) mutable { return a + ++n; };
inline auto mut2 = [n = 0](int a, int b) mutable { return a + b + ++n; };
using NC = rg::filter_view<V, decltype(pred)>;  // forward, not const-iterable

template <class R>
constexpr bool const_iterable = rg::range<const R>;

static_assert(rg::forward_range<NC> && !const_iterable<NC>);
static_assert(!const_iterable<rg::drop_while_view<V, decltype(pred)>>);
static_assert(!const_iterable<rg::chunk_by_view<V, decltype(eq)>>);
static_assert(!const_iterable<rg::split_view<V, rg::single_view<int>>>);
static_assert(!const_iterable<rg::cache_latest_view<V>>);

static_assert(const_iterable<rg::lazy_split_view<V, rg::single_view<int>>>);
static_assert(!const_iterable<rg::lazy_split_view<NC, rg::single_view<int>>>);

static_assert(const_iterable<rg::slide_view<V>>);
static_assert(!const_iterable<rg::slide_view<L>>);  // bidirectional: caches its begin
static_assert(rg::forward_range<rg::slide_view<L>>);

static_assert(const_iterable<rg::stride_view<V>> && !const_iterable<rg::stride_view<NC>>);
static_assert(const_iterable<rg::as_rvalue_view<V>> && !const_iterable<rg::as_rvalue_view<NC>>);
static_assert(const_iterable<rg::as_input_view<V>> && !const_iterable<rg::as_input_view<NC>>);
static_assert(const_iterable<rg::enumerate_view<V>> && !const_iterable<rg::enumerate_view<NC>>);
static_assert(const_iterable<rg::cartesian_product_view<V, L>>);
static_assert(!const_iterable<rg::cartesian_product_view<V, NC>>);
static_assert(const_iterable<rg::concat_view<V, L>> && !const_iterable<rg::concat_view<V, NC>>);

using VV = rg::ref_view<std::vector<std::vector<int>>>;
static_assert(const_iterable<rg::join_with_view<VV, rg::single_view<int>>>);
// a range of prvalue ranges: range_reference_t<const V> is not a reference
using PV = rg::transform_view<V, decltype([](int n) { return std::vector<int>(n); })>;
static_assert(!const_iterable<rg::join_with_view<PV, rg::single_view<int>>>);
static_assert(rg::input_range<rg::join_with_view<PV, rg::single_view<int>>>);

static_assert(!const_iterable<decltype(std::declval<V>() | vw::transform(mut))>);
static_assert(const_iterable<decltype(std::declval<V>() | vw::transform([](int a) { return a; }))>);
static_assert(const_iterable<decltype(vw::zip_transform(plus, std::declval<V>(), std::declval<L>()))>);
static_assert(!const_iterable<decltype(vw::zip_transform(mut2, std::declval<V>(), std::declval<L>()))>);
static_assert(const_iterable<decltype(std::declval<V>() | vw::pairwise_transform(plus))>);
static_assert(!const_iterable<decltype(std::declval<V>() | vw::pairwise_transform(mut2))>);

int main() {}
