// [alg.sorting]: the ranges algorithms are constrained on their iterator categories and on
// sortable / mergeable / indirect_strict_weak_order: ranges::sort, stable_sort,
// partial_sort, nth_element and the heap operations need random_access_iterator;
// inplace_merge, stable_partition and next/prev_permutation need bidirectional_iterator;
// partition, binary search, is_sorted and min_element need forward_iterator; merge, set
// operations, includes, lexicographical_compare and partition_copy accept input_iterator.
// A comparator that is not a strict weak order over the projected values is rejected.
// COUNTERPART: libcxx:algorithms/alg.sorting/alg.sort/sort/sort_constexpr(_comp)?.pass.cpp
#include <algorithm>
#include <functional>
#include <ranges>
#include "test_iterators.hpp"

template <class R>
concept can_sort = requires(R& r) { std::ranges::sort(r); };
template <class R>
concept can_stable_sort = requires(R& r) { std::ranges::stable_sort(r); };
template <class R>
concept can_make_heap = requires(R& r) { std::ranges::make_heap(r); };
template <class R>
concept can_nth = requires(R& r) { std::ranges::nth_element(r, std::ranges::begin(r)); };
template <class R>
concept can_partial_sort = requires(R& r) { std::ranges::partial_sort(r, std::ranges::begin(r)); };
template <class R>
concept can_inplace_merge = requires(R& r) { std::ranges::inplace_merge(r, std::ranges::begin(r)); };
template <class R>
concept can_stable_partition = requires(R& r) { std::ranges::stable_partition(r, [](int) { return true; }); };
template <class R>
concept can_next_perm = requires(R& r) { std::ranges::next_permutation(r); };
template <class R>
concept can_partition = requires(R& r) { std::ranges::partition(r, [](int) { return true; }); };
template <class R>
concept can_lower_bound = requires(R& r) { std::ranges::lower_bound(r, 1); };
template <class R>
concept can_is_sorted = requires(R& r) { std::ranges::is_sorted(r); };
template <class R>
concept can_min_element = requires(R& r) { std::ranges::min_element(r); };
template <class R>
concept can_merge = requires(R& r, int* o) { std::ranges::merge(r, r, o); };
template <class R>
concept can_set_union = requires(R& r, int* o) { std::ranges::set_union(r, r, o); };
template <class R>
concept can_includes = requires(R& r) { std::ranges::includes(r, r); };
template <class R>
concept can_lex = requires(R& r) { std::ranges::lexicographical_compare(r, r); };
template <class R>
concept can_min = requires(R& r) { std::ranges::min(r); };

using In = InputRange<int>;
using Fwd = ForwardRange<int>;
using Bidi = BidiRange<int>;
using Rnd = RandomRange<int>;

static_assert(can_sort<Rnd> && !can_sort<Bidi>);
static_assert(can_stable_sort<Rnd> && !can_stable_sort<Bidi>);
static_assert(can_make_heap<Rnd> && !can_make_heap<Bidi>);
static_assert(can_nth<Rnd> && !can_nth<Bidi>);
static_assert(can_partial_sort<Rnd> && !can_partial_sort<Bidi>);
static_assert(can_inplace_merge<Bidi> && !can_inplace_merge<Fwd>);
static_assert(can_stable_partition<Bidi> && !can_stable_partition<Fwd>);
static_assert(can_next_perm<Bidi> && !can_next_perm<Fwd>);
static_assert(can_partition<Fwd> && !can_partition<In>);
static_assert(can_lower_bound<Fwd> && !can_lower_bound<In>);
static_assert(can_is_sorted<Fwd> && !can_is_sorted<In>);
static_assert(can_min_element<Fwd> && !can_min_element<In>);
static_assert(can_merge<In>);
static_assert(can_set_union<In>);
static_assert(can_includes<In>);
static_assert(can_lex<In>);
static_assert(can_min<In>);

// sortable requires a strict weak order over the projected values
struct NoLess {
  int v;
};
template <class R, class C = std::ranges::less, class P = std::identity>
concept can_sort_with = requires(R& r, C c, P p) { std::ranges::sort(r, c, p); };
template <class R>
concept can_partition_any = requires(R& r) { std::ranges::partition(r, [](int) { return true; }); };
template <class R>
concept can_is_sorted_any = requires(R& r) { std::ranges::is_sorted(r); };
struct NoLessCmp {
  bool operator()(const NoLess&, const NoLess&) const { return true; }
};

static_assert(!can_sort_with<NoLess[3]>);
static_assert(can_sort_with<NoLess[3], std::ranges::less, int NoLess::*>);
// a comparator that is not callable with the projected type
static_assert(!can_sort_with<int[3], NoLessCmp>);
// const elements are not sortable / permutable
static_assert(!can_sort_with<const int[3]>);
static_assert(!can_partition_any<const int[3]>);
static_assert(can_is_sorted_any<const int[3]>);

int main() { return 0; }
