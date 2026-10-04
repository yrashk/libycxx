// [range.prim.size.hint]/2: ranges::reserve_hint(E) is ranges::size(E) if that is valid;
// otherwise auto(t.reserve_hint()) if that is a valid expression of integer-like type;
// otherwise auto(reserve_hint(t)) found by argument-dependent lookup only, if of integer-like
// type; otherwise ill-formed (substitution failure). ([iterator.concept.winc]: bool is not
// integer-like.) [range.approximately.sized]/1: approximately_sized_range<T> is range<T> &&
// requires(T& t) { ranges::reserve_hint(t); }; [range.sized]/1: sized_range refines it.
// The views forward or derive the hint: ref_view / owning_view / as_rvalue_view /
// transform_view / as_const_view / enumerate_view / cache_latest_view / as_input_view return
// the base's ([range.ref.view], [range.owning.view], [range.as.rvalue.view],
// [range.transform.view], [range.as.const.view], [range.enumerate.view],
// [range.cache.latest.view]/5, [range.as.input.view]/6); take_view: min(hint, count), or
// count for a base without a hint ([range.take.view]); drop_view: hint - count, at least 0
// ([range.drop.view]); adjacent_view<V, N>: hint - (N - 1), at least 0
// ([range.adjacent.view]/3); chunk_view and stride_view: div-ceil(hint, n)
// ([range.chunk.view.input]/6, [range.chunk.view.fwd]/4, [range.stride.view]/5); slide_view:
// hint - n + 1, at least 0 ([range.slide.view]/10); concat_view: the sum
// ([range.concat.view]/9). [range.utility.conv.to]/2.1.4: ranges::to into a
// reservable-container that is filled by appending calls c.reserve(ranges::reserve_hint(r))
// for an approximately sized r ([range.utility.conv.general]/3).
#include <ranges>
#include <cstddef>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

// A forward view (not sized) with a member reserve_hint.
struct HintedView : rg::view_base {
  int* b = nullptr;
  int* e = nullptr;
  std::size_t hint = 0;
  HintedView() = default;
  constexpr HintedView(int* b_, int* e_, std::size_t h) : b(b_), e(e_), hint(h) {}
  constexpr ForwardIter<int> begin() const { return ForwardIter<int>(b); }
  constexpr PtrSentinel<int> end() const { return PtrSentinel<int>{e}; }
  constexpr std::size_t reserve_hint() const { return hint; }
};

// An input view (not sized, no hint).
struct PlainInput : rg::view_base {
  int* b = nullptr;
  int* e = nullptr;
  PlainInput() = default;
  constexpr PlainInput(int* b_, int* e_) : b(b_), e(e_) {}
  constexpr InputIter<int> begin() const { return InputIter<int>(b); }
  constexpr PtrSentinel<int> end() const { return PtrSentinel<int>{e}; }
};

namespace adl {
struct AdlHinted {
  int* b;
  int* e;
  constexpr ForwardIter<int> begin() const { return ForwardIter<int>(b); }
  constexpr PtrSentinel<int> end() const { return PtrSentinel<int>{e}; }
  friend constexpr long reserve_hint(const AdlHinted&) { return 42; }
};
}  // namespace adl

struct SizedAndHinted {
  int a[3];
  constexpr const int* begin() const { return a; }
  constexpr const int* end() const { return a + 3; }
  constexpr std::size_t reserve_hint() const { return 100; }  // size() takes precedence
};

struct BoolHint {
  int* b;
  int* e;
  constexpr ForwardIter<int> begin() const { return ForwardIter<int>(b); }
  constexpr PtrSentinel<int> end() const { return PtrSentinel<int>{e}; }
  constexpr bool reserve_hint() const { return true; }
};

struct DoubleHint {
  int* b;
  int* e;
  constexpr ForwardIter<int> begin() const { return ForwardIter<int>(b); }
  constexpr PtrSentinel<int> end() const { return PtrSentinel<int>{e}; }
  constexpr double reserve_hint() const { return 2.0; }
};

template <class T>
concept hintable = requires(T& t) { rg::reserve_hint(t); };

static_assert(rg::approximately_sized_range<HintedView> && !rg::sized_range<HintedView>);
static_assert(!rg::approximately_sized_range<PlainInput>);
static_assert(rg::approximately_sized_range<adl::AdlHinted>);
static_assert(!hintable<BoolHint> && !rg::approximately_sized_range<BoolHint>);
static_assert(!hintable<DoubleHint>);
static_assert(rg::approximately_sized_range<int[4]> && rg::sized_range<int[4]>);
static_assert(std::is_same_v<decltype(rg::reserve_hint(std::declval<adl::AdlHinted&>())), long>);
static_assert(std::is_same_v<decltype(rg::reserve_hint(std::declval<HintedView&>())), std::size_t>);
static_assert(std::is_nothrow_invocable_v<decltype(rg::reserve_hint), int (&)[4]>);

// Records reserve() calls; filled by push_back.
struct ResCont {
  int data[64] = {};
  std::size_t n = 0;
  std::size_t reserved = 0;
  int reserve_calls = 0;
  ResCont() = default;
  constexpr const int* begin() const { return data; }
  constexpr const int* end() const { return data + n; }
  constexpr int* begin() { return data; }
  constexpr int* end() { return data + n; }
  constexpr std::size_t size() const { return n; }
  constexpr void reserve(std::size_t k) {
    reserved = k;
    ++reserve_calls;
  }
  constexpr std::size_t capacity() const { return 64; }
  constexpr std::size_t max_size() const { return 64; }
  constexpr void push_back(int x) { data[n++] = x; }
};

constexpr bool test() {
  int a[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  HintedView h(a, a + 10, 10);  // an accurate hint
  if (rg::reserve_hint(h) != 10) return false;
  if (rg::reserve_hint(a) != 10) return false;  // via size
  SizedAndHinted sh{{1, 2, 3}};
  if (rg::reserve_hint(sh) != 3) return false;
  adl::AdlHinted ah{a, a + 2};
  if (rg::reserve_hint(ah) != 42) return false;

  if (rg::reserve_hint(vw::all(h)) != 10) return false;
  if (rg::reserve_hint(rg::ref_view(h)) != 10) return false;
  if (rg::reserve_hint(h | vw::transform([](int x) { return x * 2; })) != 10) return false;
  if (rg::reserve_hint(h | vw::as_rvalue) != 10) return false;
  if (rg::reserve_hint(h | vw::as_const) != 10) return false;
  if (rg::reserve_hint(h | vw::enumerate) != 10) return false;
  if (rg::reserve_hint(h | vw::cache_latest) != 10) return false;
  if (rg::reserve_hint(h | vw::as_input) != 10) return false;
  if (rg::reserve_hint(vw::take(h, 4)) != 4) return false;
  if (rg::reserve_hint(vw::take(h, 40)) != 10) return false;
  if (rg::reserve_hint(rg::take_view(PlainInput(a, a + 10), 3)) != 3) return false;  // count
  static_assert(rg::approximately_sized_range<rg::take_view<PlainInput>>);
  if (rg::reserve_hint(vw::drop(h, 3)) != 7) return false;
  if (rg::reserve_hint(vw::drop(h, 30)) != 0) return false;
  if (rg::reserve_hint(h | vw::adjacent<3>) != 8) return false;
  if (rg::reserve_hint(HintedView(a, a + 1, 1) | vw::adjacent<3>) != 0) return false;
  if (rg::reserve_hint(h | vw::pairwise_transform([](int x, int y) { return x + y; })) != 9) return false;
  if (rg::reserve_hint(vw::chunk(h, 3)) != 4) return false;
  static_assert(!rg::approximately_sized_range<decltype(vw::chunk(std::declval<PlainInput>(), 3))>);
  if (rg::reserve_hint(vw::slide(h, 4)) != 7) return false;
  if (rg::reserve_hint(vw::slide(h, 20)) != 0) return false;
  if (rg::reserve_hint(vw::stride(h, 3)) != 4) return false;
  if (rg::reserve_hint(vw::concat(h, HintedView(a, a + 3, 3))) != 13) return false;
  static_assert(!rg::approximately_sized_range<decltype(vw::concat(std::declval<HintedView&>(),
                                                                   std::declval<PlainInput&>()))>);
  static_assert(!rg::approximately_sized_range<decltype(std::declval<PlainInput&>() | vw::as_input)>);

  // A wrong hint flows through unchanged (it is only a hint).
  HintedView liar(a, a + 10, 3);
  if (rg::reserve_hint(liar | vw::transform([](int x) { return x; })) != 3) return false;
  if (rg::distance(liar) != 10) return false;

  // ranges::to reserves the hint for a reservable container filled by appending
  ResCont c = rg::to<ResCont>(liar);
  if (c.reserve_calls != 1 || c.reserved != 3 || c.n != 10 || c.data[9] != 9) return false;
  ResCont d = rg::to<ResCont>(PlainInput(a, a + 5));  // no hint: no reserve
  if (d.reserve_calls != 0 || d.n != 5) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
