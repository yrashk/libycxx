// [range.iota.view], [range.iota.iterator]: the concepts iota_view models, its iterator's
// iterator_concept (/1: advanceable -> random_access, decrementable -> bidirectional,
// incrementable -> forward, otherwise input), the conditional iterator_category ("present
// only if W models incrementable and IOTA-DIFF-T(W) is an integral type"), and
// IOTA-DIFF-T ([range.iota.view]/1).
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include "range_support.hpp"

namespace rg = std::ranges;

// Bounded integer iota: random access, common, sized, borrowed.
using IV = rg::iota_view<int, int>;
static_assert(rg::view<IV> && rg::random_access_range<IV> && rg::common_range<IV>);
static_assert(rg::sized_range<IV> && rg::borrowed_range<IV>);
static_assert(rg::random_access_range<const IV> && rg::sized_range<const IV>);
static_assert(!rg::contiguous_range<IV>);
static_assert(std::is_same_v<rg::range_value_t<IV>, int>);
static_assert(std::is_same_v<rg::range_reference_t<IV>, int>); // prvalue W
using IVI = rg::iterator_t<IV>;
static_assert(std::is_same_v<IVI::iterator_concept, std::random_access_iterator_tag>);
static_assert(std::is_same_v<IVI::iterator_category, std::input_iterator_tag>);
static_assert(std::is_same_v<IVI::value_type, int>);
// /1.2: a signed integer type wider than int.
static_assert(std::signed_integral<IVI::difference_type> && sizeof(IVI::difference_type) > sizeof(int));
static_assert(std::is_same_v<decltype(IV().end()), IVI>);
static_assert(noexcept(*std::declval<const IVI&>()));
static_assert(std::is_same_v<decltype(IV().size()), unsigned int>);

// Unsigned W: difference_type is still signed.
using UV = rg::iota_view<unsigned, unsigned>;
static_assert(std::signed_integral<rg::range_difference_t<UV>>);
static_assert(sizeof(rg::range_difference_t<UV>) > sizeof(unsigned));
static_assert(std::is_same_v<decltype(UV().size()), unsigned>);
static_assert(std::is_same_v<decltype(rg::iota_view<long long, long long>().size()), unsigned long long>);

// Unbounded: not common, not sized; end() is unreachable_sentinel.
using UB = rg::iota_view<int>;
static_assert(std::is_same_v<UB, rg::iota_view<int, std::unreachable_sentinel_t>>);
static_assert(rg::random_access_range<UB> && !rg::common_range<UB> && !rg::sized_range<UB>);
static_assert(std::is_same_v<rg::sentinel_t<UB>, std::unreachable_sentinel_t>);
static_assert(rg::borrowed_range<UB>);

// Mixed bound: iota_view<int, long> uses a separate sentinel type.
using MB = rg::iota_view<int, long>;
static_assert(rg::random_access_range<MB> && !rg::common_range<MB> && rg::sized_range<MB>);

// Pointer W: IOTA-DIFF-T(int*) is ptrdiff_t (/1.1); random access since int* is advanceable.
using PV = rg::iota_view<int*, int*>;
static_assert(std::is_same_v<rg::range_difference_t<PV>, std::ptrdiff_t>);
static_assert(std::is_same_v<rg::iterator_t<PV>::iterator_concept, std::random_access_iterator_tag>);
static_assert(std::is_same_v<rg::range_reference_t<PV>, int*>);
static_assert(rg::sized_range<PV>);

// Bidirectional W (decrementable but not advanceable).
struct Bidi {
  int v = 0;
  using difference_type = int;
  constexpr Bidi& operator++() { ++v; return *this; }
  constexpr Bidi operator++(int) { auto t = *this; ++v; return t; }
  constexpr Bidi& operator--() { --v; return *this; }
  constexpr Bidi operator--(int) { auto t = *this; --v; return t; }
  friend constexpr bool operator==(Bidi, Bidi) = default;
};
using BV = rg::iota_view<Bidi, Bidi>;
static_assert(rg::bidirectional_range<BV> && !rg::random_access_range<BV> && rg::common_range<BV>);
static_assert(std::is_same_v<rg::iterator_t<BV>::iterator_concept, std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<BV>::iterator_category, std::input_iterator_tag>);
static_assert(!rg::sized_range<BV>);

// Forward W (incrementable, not decrementable).
struct Fwd {
  int v = 0;
  using difference_type = int;
  constexpr Fwd& operator++() { ++v; return *this; }
  constexpr Fwd operator++(int) { auto t = *this; ++v; return t; }
  friend constexpr bool operator==(Fwd, Fwd) = default;
};
using FV = rg::iota_view<Fwd, Fwd>;
static_assert(rg::forward_range<FV> && !rg::bidirectional_range<FV>);
static_assert(std::is_same_v<rg::iterator_t<FV>::iterator_concept, std::forward_iterator_tag>);
static_assert(std::is_same_v<decltype(std::declval<rg::iterator_t<FV>&>()++), rg::iterator_t<FV>>);

// Input-only W (weakly_incrementable, not incrementable: no equality, void postfix ++).
struct In {
  int v = 0;
  using difference_type = int;
  constexpr In& operator++() { ++v; return *this; }
  constexpr void operator++(int) { ++v; }
};
using InV = rg::iota_view<In>;
static_assert(rg::input_range<InV> && !rg::forward_range<InV>);
static_assert(std::is_same_v<rg::iterator_t<InV>::iterator_concept, std::input_iterator_tag>);
static_assert(!has_iterator_category<rg::iterator_t<InV>>);
static_assert(std::is_void_v<decltype(std::declval<rg::iterator_t<InV>&>()++)>);

// The deduction guide rejects mixed-signedness integer arguments.
template <class A, class B>
concept iota_deducible = requires(A a, B b) { rg::iota_view(a, b); };
static_assert(iota_deducible<int, long>);
static_assert(iota_deducible<unsigned, unsigned long>);
static_assert(!iota_deducible<int, unsigned>);
static_assert(!iota_deducible<unsigned long, long>);

// views::iota(E) is iota_view<decay_t<decltype((E))>>.
static_assert(std::is_same_v<decltype(std::views::iota(0)), rg::iota_view<int>>);
static_assert(std::is_same_v<decltype(std::views::iota(0, 5L)), rg::iota_view<int, long>>);
static_assert(std::is_same_v<decltype(std::views::iota('a', 'z')), rg::iota_view<char, char>>);
