// Generic check, instantiated per associative / unordered associative / flat container: the
// range members accept every kind of range that models container-compatible-range<value_type>
// ([container.intro.reqmts]/2: "ranges::input_range<R> &&
// convertible_to<ranges::range_reference_t<R>, T>"), not only common forward ranges.
//   [associative.reqmts.general]/29-34: X(from_range, rg, c), X(from_range, rg): "Constructs
//     an empty container and inserts each element from rg into it."
//   [associative.reqmts.general]/80-83: a.insert_range(rg): "Inserts each element from rg if
//     and only if there is no element with key equivalent to the key of that element in
//     containers with unique keys; always inserts that element in containers with
//     equivalent keys."
//   [unord.req.general]: X(from_range, rg, n, hf, eq), X(from_range, rg, n, hf),
//     X(from_range, rg, n), X(from_range, rg) and a.insert_range(rg), with the same effects.
//   [flat.map.modifiers]/11-15, [flat.set.modifiers], [flat.multimap.defn],
//     [flat.multiset.defn]: insert_range(rg), insert_range(sorted_unique, rg) /
//     insert_range(sorted_equivalent, rg) (equivalent to insert_range(rg)).
// Range kinds: sized but input-only ranges (a member size(), views::counted over an input
// iterator); approximately-sized ranges ([range.approximately.sized]) whose reserve_hint() is
// wrong in either direction (only an approximation); a plain input range; a range of
// prvalues (iota | transform); a move-only view passed as an rvalue; a view that can only be
// iterated when non-const; for maps, ranges of tuple<int, int> and array<int, 2> (pair-like:
// pair's converting constructor from pair-like types, [pairs.pair]/14-16, makes them
// convertible to value_type); and, for move-only element types, views::as_rvalue and a
// transform producing prvalues (value_type need only be Cpp17EmplaceConstructible from
// *ranges::begin(rg)).
#pragma once
#include <array>
#include <cstddef>
#include <flat_set>  // std::sorted_unique / sorted_equivalent, named for every container kind
#include <iterator>
#include <ranges>
#include <tuple>
#include <utility>
#include "assoc_common.hpp"
#include "reqs/sequence_range_kinds.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace reqs::assoc_range_kinds {

using namespace reqs::assoc;
using reqs::sequence_range_kinds::HintedRange;
using reqs::sequence_range_kinds::SizedInputRange;

// sorted_unique / sorted_equivalent live in <flat_map> and <flat_set> ([flat.map.syn],
// [flat.set.syn]); only the flat containers' tests instantiate sorted_tag, so the other
// containers' tests do not depend on those headers.
template <class X>
constexpr auto sorted_tag();
}  // namespace reqs::assoc_range_kinds
#if __has_include(<flat_set>)
#include <flat_set>
namespace reqs::assoc_range_kinds {
template <class X>
constexpr auto sorted_tag() {
  if constexpr (is_multi<X>) return std::sorted_equivalent;
  else return std::sorted_unique;
}
}  // namespace reqs::assoc_range_kinds
#endif
namespace reqs::assoc_range_kinds {

// X(from_range, rg) and, for the unordered containers, X(from_range, rg, n) as well.
template <class X, class R>
constexpr X build(R&& rg) {
  return X(std::from_range, std::forward<R>(rg));
}

template <class X>
constexpr bool copyable() {
  using S = src_t<X>;
  // 2 and 5 appear twice: one element each for unique keys, two for equivalent keys
  S s[10] = {src<X>(5), src<X>(1), src<X>(4), src<X>(2), src<X>(5),
             src<X>(9), src<X>(2), src<X>(7), src<X>(0), src<X>(3)};
  const bool multi = is_multi<X>;
  auto all10 = [&](const X& x) {
    return multi ? contents(x, {5, 1, 4, 2, 5, 9, 2, 7, 0, 3}) : contents(x, {5, 1, 4, 2, 9, 7, 0, 3});
  };
  {
    X x = build<X>(SizedInputRange<S>{s, s + 10});
    if (!all10(x)) return false;
    X y = build<X>(std::views::counted(InputIter<S>(s + 1), 3));
    if (!contents(y, {1, 4, 2})) return false;
    X z = build<X>(InputRange<S>{s, s + 10});
    if (!all10(z)) return false;
  }
  {
    X x = build<X>(HintedRange<InputIter, S>{s, s + 10, 1});
    if (!all10(x)) return false;
    X y = build<X>(HintedRange<InputIter, S>{s + 1, s + 3, 500});
    if (!contents(y, {1, 4})) return false;
    X z = build<X>(HintedRange<ForwardIter, S>{s, s + 4, 0});
    if (!contents(z, {5, 1, 4, 2})) return false;
    X w = build<X>(HintedRange<ForwardIter, S>{s, s, 30});
    if (!(w.begin() == w.end()) || !w.empty()) return false;
  }
  {
    X x = build<X>(std::views::iota(3, 6) | std::views::transform([](int k) { return src<X>(k); }));
    if (!contents(x, {3, 4, 5})) return false;
    X y = build<X>(MoveOnlyView<S>(s, s + 4));
    if (!contents(y, {5, 1, 4, 2})) return false;
    X z = build<X>(MutableOnlyView<S>(s + 5, s + 8));
    if (!contents(z, {9, 2, 7})) return false;
  }
  if constexpr (is_unordered<X>) {
    X x(std::from_range, SizedInputRange<S>{s, s + 10}, 64);
    if (!all10(x) || x.bucket_count() < 64) return false;
    X y(std::from_range, InputRange<S>{s, s + 3}, 1, typename X::hasher());
    if (!contents(y, {5, 1, 4})) return false;
    X z(std::from_range, MoveOnlyView<S>(s, s + 2), 3, typename X::hasher(), typename X::key_equal());
    if (!contents(z, {5, 1})) return false;
  } else {
    X x(std::from_range, InputRange<S>{s, s + 10}, typename X::key_compare());
    if (!all10(x)) return false;
  }
  if constexpr (is_map<X>) {  // pair-like elements
    std::tuple<int, int> t[3] = {{8, 108}, {6, 106}, {8, 108}};
    X x = build<X>(t);
    if (!(multi ? contents(x, {8, 6, 8}) : contents(x, {8, 6}))) return false;
    std::array<int, 2> a[2] = {{{11, 111}}, {{10, 110}}};
    x.insert_range(a);
    if (!(multi ? contents(x, {8, 6, 8, 11, 10}) : contents(x, {8, 6, 11, 10}))) return false;
    x.insert_range(SizedInputRange<std::tuple<int, int>>{t + 1, t + 2});
    if (!(multi ? contents(x, {8, 6, 8, 11, 10, 6}) : contents(x, {8, 6, 11, 10}))) return false;
  }
  {  // insert_range into a non-empty container, overlapping keys
    X x = build<X>(std::views::counted(InputIter<S>(s), 2));  // 5, 1
    x.insert_range(SizedInputRange<S>{s + 1, s + 4});          // 1, 4, 2
    if (!(multi ? contents(x, {5, 1, 1, 4, 2}) : contents(x, {5, 1, 4, 2}))) return false;
    x.insert_range(HintedRange<InputIter, S>{s + 5, s + 7, 100});  // 9, 2
    x.insert_range(std::views::counted(InputIter<S>(s + 7), 1));   // 7
    x.insert_range(InputRange<S>{s + 8, s + 10});                  // 0, 3
    x.insert_range(MoveOnlyView<S>(s, s + 1));                     // 5
    x.insert_range(MutableOnlyView<S>(s, s));                      // nothing
    x.insert_range(std::views::iota(12, 14) | std::views::transform([](int k) { return src<X>(k); }));
    if (!(multi ? contents(x, {5, 1, 1, 4, 2, 9, 2, 7, 0, 3, 5, 12, 13})
                : contents(x, {5, 1, 4, 2, 9, 7, 0, 3, 12, 13})))
      return false;
  }
  if constexpr (is_flat<X>) {  // insert_range(sorted_unique / sorted_equivalent, rg)
    S sorted[4] = {src<X>(1), src<X>(3), src<X>(6), src<X>(8)};
    X x = build<X>(std::views::counted(InputIter<S>(s + 1), 2));  // 1, 4
    x.insert_range(sorted_tag<X>(), SizedInputRange<S>{sorted, sorted + 4});
    if (!(multi ? contents(x, {1, 4, 1, 3, 6, 8}) : contents(x, {1, 4, 3, 6, 8}))) return false;
    x.insert_range(sorted_tag<X>(), InputRange<S>{sorted + 2, sorted + 4});
    if (!(multi ? contents(x, {1, 4, 1, 3, 6, 8, 6, 8}) : contents(x, {1, 4, 3, 6, 8}))) return false;
  }
  return true;
}

// X has a move-only element: map<int, MOElem> / set<MOElem> / ...
template <class X>
constexpr bool move_only() {
  using V = typename X::value_type;
  auto make = [](int k) { return elem<X>(k); };
  V arr[3] = {make(4), make(2), make(7)};
  X x(std::from_range, std::views::as_rvalue(arr));
  if (!contents(x, {4, 2, 7})) return false;
  for (const V& v : arr) {  // every source element was moved from
    if constexpr (is_map<X>) {
      if (value_of(v.second) != -1) return false;
    } else {
      if (value_of(v) != -1) return false;
    }
  }
  x.insert_range(std::views::iota(10, 13) | std::views::transform(make));
  if (!contents(x, {4, 2, 7, 10, 11, 12})) return false;
  V more[2] = {make(1), make(20)};
  x.insert_range(std::views::as_rvalue(more));
  if (!contents(x, {4, 2, 7, 10, 11, 12, 1, 20})) return false;
  X y(std::from_range, std::views::iota(0, 2) | std::views::transform(make));
  return contents(y, {0, 1});
}

}  // namespace reqs::assoc_range_kinds
