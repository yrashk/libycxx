// Generic check, instantiated per sequence container: the range members accept every kind of
// range that models container-compatible-range<T> ([container.intro.reqmts]/2:
// "ranges::input_range<R> && convertible_to<ranges::range_reference_t<R>, T>"), not only the
// common forward ones. [sequence.reqmts]/11-14 (X(from_range, rg): "Constructs a sequence
// container equal to the range rg. Each iterator in the range rg is dereferenced exactly
// once." Postcondition: distance(begin(), end()) == ranges::distance(rg)), /40-43
// (a.insert_range(p, rg): iterator to the first inserted element, or p if rg is empty),
// /60-64 (a.assign_range(rg)), /109-111 (a.append_range(rg)), prepend_range, and
// [forward.list.modifiers] insert_range_after (iterator to the last inserted element, or p).
// Range kinds:
//   - sized_range but only an input range (a member size(), and views::counted over an
//     input iterator);
//   - approximately_sized_range ([range.approximately.sized], [range.prim.size.hint]: a
//     member reserve_hint()) whose hint is too small or too large: the hint is only an
//     approximation (/13, /64 "Recommended practice ... if ranges::distance(rg) <=
//     ranges::reserve_hint(rg)"), so the result must not depend on it;
//   - a range whose reference is a different type convertible to T, or a prvalue (iota,
//     transform);
//   - a move-only view passed as an rvalue, and a view that can only be iterated when not
//     const;
//   - for a move-only T: views::as_rvalue (reference T&&) and a transform producing T
//     prvalues; T need only be Cpp17EmplaceConstructible from *ranges::begin(rg) (plus
//     Cpp17MoveInsertable etc. where /41, /62, /110 say so).
// C<T> names the container type for element type T.
#pragma once
#include <cstddef>
#include <iterator>
#include <ranges>
#include <utility>
#include "container_values.hpp"
#include "move_only_elem.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace reqs::sequence_range_kinds {

// sized_range, input-only, not common.
template <class T>
struct SizedInputRange {
  T* b;
  T* e;
  constexpr InputIter<T> begin() const { return InputIter<T>(b); }
  constexpr PtrSentinel<T> end() const { return PtrSentinel<T>{e}; }
  constexpr std::size_t size() const { return static_cast<std::size_t>(e - b); }
};

// approximately_sized_range (not sized) with an arbitrary hint; It is InputIter or ForwardIter.
template <template <class> class It, class T>
struct HintedRange {
  T* b;
  T* e;
  std::size_t hint;
  constexpr It<T> begin() const { return It<T>(b); }
  constexpr PtrSentinel<T> end() const { return PtrSentinel<T>{e}; }
  constexpr std::size_t reserve_hint() const { return hint; }
};

static_assert(std::ranges::sized_range<SizedInputRange<int>> && !std::ranges::forward_range<SizedInputRange<int>>);
static_assert(std::ranges::sized_range<decltype(std::views::counted(InputIter<int>(), 0))>);
static_assert(!std::ranges::forward_range<decltype(std::views::counted(InputIter<int>(), 0))>);
static_assert(!std::ranges::sized_range<HintedRange<InputIter, int>>);

template <class X>
constexpr bool is_forward_list = requires(X& x) { x.before_begin(); };

// Inserts rg so that its first element becomes element k; returns the member's result.
template <class X, class R>
constexpr auto insert_at(X& x, int k, R&& rg) {
  if constexpr (is_forward_list<X>) {
    auto p = x.cbefore_begin();
    for (int i = 0; i < k; ++i) ++p;
    return x.insert_range_after(p, std::forward<R>(rg));
  } else {
    return x.insert_range(cnth(x, k), std::forward<R>(rg));
  }
}

template <template <class> class C>
constexpr bool copyable_elements() {
  using X = C<Elem>;  // Elem is implicitly convertible from int: ranges of int are compatible
  int src[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  {
    X x(std::from_range, SizedInputRange<int>{src, src + 4});
    if (!values_are(x, {0, 1, 2, 3})) return false;
  }
  {
    X x(std::from_range, std::views::counted(InputIter<int>(src + 2), 3));
    if (!values_are(x, {2, 3, 4})) return false;
  }
  {
    X x(std::from_range, HintedRange<InputIter, int>{src, src + 10, 1});
    if (!values_are(x, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9})) return false;
    X y(std::from_range, HintedRange<InputIter, int>{src, src + 2, 50});
    if (!values_are(y, {0, 1})) return false;
    X z(std::from_range, HintedRange<ForwardIter, int>{src, src + 7, 2});
    if (!values_are(z, {0, 1, 2, 3, 4, 5, 6})) return false;
    X w(std::from_range, HintedRange<ForwardIter, int>{src, src, 30});
    if (!(w.begin() == w.end())) return false;
  }
  {
    X x(std::from_range, std::views::iota(5, 8));
    if (!values_are(x, {5, 6, 7})) return false;
    X y(std::from_range, MoveOnlyView<int>(src, src + 3));
    if (!values_are(y, {0, 1, 2})) return false;
    X z(std::from_range, MutableOnlyView<int>(src + 1, src + 3));
    if (!values_are(z, {1, 2})) return false;
  }
  {  // assign_range
    X x(std::from_range, SizedInputRange<int>{src, src + 6});
    x.assign_range(SizedInputRange<int>{src + 7, src + 10});
    if (!values_are(x, {7, 8, 9})) return false;
    x.assign_range(HintedRange<InputIter, int>{src, src + 8, 2});
    if (!values_are(x, {0, 1, 2, 3, 4, 5, 6, 7})) return false;
    x.assign_range(HintedRange<ForwardIter, int>{src + 8, src + 10, 40});
    if (!values_are(x, {8, 9})) return false;
    x.assign_range(std::views::iota(1, 4));
    if (!values_are(x, {1, 2, 3})) return false;
    x.assign_range(MoveOnlyView<int>(src + 4, src + 5));
    if (!values_are(x, {4})) return false;
  }
  {  // insert_range / insert_range_after
    X x(std::from_range, std::views::iota(100, 104));
    auto r = insert_at(x, 2, SizedInputRange<int>{src, src + 3});
    if (!values_are(x, {100, 101, 0, 1, 2, 102, 103})) return false;
    if (value_of(*r) != (is_forward_list<X> ? 2 : 0)) return false;
    auto e = insert_at(x, 1, SizedInputRange<int>{src, src});  // empty: returns p
    if (is_forward_list<X> ? value_of(*e) != 100 : value_of(*e) != 101) return false;
    r = insert_at(x, 0, HintedRange<InputIter, int>{src + 5, src + 9, 1});
    if (!values_are(x, {5, 6, 7, 8, 100, 101, 0, 1, 2, 102, 103})) return false;
    if (value_of(*r) != (is_forward_list<X> ? 8 : 5)) return false;
    r = insert_at(x, 11, std::views::counted(InputIter<int>(src + 3), 2));
    if (!values_are(x, {5, 6, 7, 8, 100, 101, 0, 1, 2, 102, 103, 3, 4})) return false;
    if (value_of(*r) != (is_forward_list<X> ? 4 : 3)) return false;
    r = insert_at(x, 4, std::views::iota(50, 52));
    if (!values_are(x, {5, 6, 7, 8, 50, 51, 100, 101, 0, 1, 2, 102, 103, 3, 4})) return false;
    if (value_of(*r) != (is_forward_list<X> ? 51 : 50)) return false;
  }
  if constexpr (requires(X& x) { x.append_range(src); }) {
    X x(std::from_range, std::views::iota(0, 2));
    x.append_range(SizedInputRange<int>{src + 5, src + 7});
    x.append_range(HintedRange<InputIter, int>{src + 7, src + 10, 1});
    x.append_range(std::views::counted(InputIter<int>(src), 1));
    x.append_range(MoveOnlyView<int>(src + 1, src + 2));
    if (!values_are(x, {0, 1, 5, 6, 7, 8, 9, 0, 1})) return false;
  }
  if constexpr (requires(X& x) { x.prepend_range(src); }) {
    X x(std::from_range, std::views::iota(0, 2));
    x.prepend_range(SizedInputRange<int>{src + 5, src + 7});
    x.prepend_range(HintedRange<InputIter, int>{src + 7, src + 10, 1});
    x.prepend_range(std::views::counted(InputIter<int>(src + 3), 1));
    if (!values_are(x, {3, 7, 8, 9, 5, 6, 0, 1})) return false;
  }
  return true;
}

template <template <class> class C>
constexpr bool move_only_elements() {
  using X = C<MOElem>;
  auto make10 = [](int i) { return MOElem(i * 10); };
  {
    MOElem arr[4] = {1, 2, 3, 4};
    X x(std::from_range, std::views::as_rvalue(arr));
    if (!values_are(x, {1, 2, 3, 4})) return false;
    for (const MOElem& m : arr)
      if (m.value() != -1) return false;  // moved from
  }
  X y(std::from_range, std::views::iota(0, 3) | std::views::transform(make10));
  if (!values_are(y, {0, 10, 20})) return false;
  {
    MOElem more[3] = {7, 8, 9};
    auto r = insert_at(y, 1, std::views::as_rvalue(more));
    if (!values_are(y, {0, 7, 8, 9, 10, 20})) return false;
    if (r->value() != (is_forward_list<X> ? 9 : 7)) return false;
    insert_at(y, 6, std::views::iota(5, 6) | std::views::transform(make10));
    if (!values_are(y, {0, 7, 8, 9, 10, 20, 50})) return false;
  }
  if constexpr (requires(MOElem (&a)[1]) { y.append_range(std::views::as_rvalue(a)); }) {
    MOElem tail[2] = {5, 6};
    y.append_range(std::views::as_rvalue(tail));
    if (!values_are(y, {0, 7, 8, 9, 10, 20, 50, 5, 6})) return false;
  }
  if constexpr (requires(MOElem (&a)[1]) { y.prepend_range(std::views::as_rvalue(a)); }) {
    MOElem head[2] = {3, 4};
    y.prepend_range(std::views::as_rvalue(head));
    if (value_of(*y.begin()) != 3 || value_of(*std::next(y.begin())) != 4) return false;
  }
  {
    MOElem fresh[2] = {11, 12};
    y.assign_range(std::views::as_rvalue(fresh));
    if (!values_are(y, {11, 12})) return false;
    y.assign_range(std::views::iota(1, 4) | std::views::transform(make10));
    if (!values_are(y, {10, 20, 30})) return false;
  }
  return true;
}

template <template <class> class C>
constexpr bool test() {
  return copyable_elements<C>() && move_only_elements<C>();
}

}  // namespace reqs::sequence_range_kinds
