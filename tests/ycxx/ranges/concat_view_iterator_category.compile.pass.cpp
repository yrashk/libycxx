// [range.concat.iterator]/2: "The member typedef-name iterator_category is declared if and
// only if all-forward<Const, Views...> is modeled." It is input_iterator_tag when the
// concat reference type is not a reference (/2.1), otherwise the strongest category all
// ranges' iterator_categories derive from and the view models (/2.2.1-/2.2.4).
#include <array>
#include <iterator>
#include <ranges>
#include <type_traits>
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

using C1 = decltype(vw::concat(std::declval<int (&)[3]>(), std::declval<std::array<int, 2>&>()));
static_assert(has_iterator_category<rg::iterator_t<C1>>);
static_assert(std::is_same_v<rg::iterator_t<C1>::iterator_category, std::random_access_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<const C1>::iterator_category, std::random_access_iterator_tag>);

// int& and long&: common reference long, a prvalue.
using C2 = decltype(vw::concat(std::declval<int (&)[2]>(), std::declval<long (&)[2]>()));
static_assert(std::is_same_v<rg::iterator_t<C2>::iterator_category, std::input_iterator_tag>);

// A non-common first range: the view is only forward, so forward_iterator_tag (/2.2.3).
using RandNC = ArchetypeView<RandomIter<int>, PtrSentinel<int>>;
using C3 = rg::concat_view<RandNC, rg::ref_view<int[2]>>;
static_assert(std::is_same_v<rg::iterator_t<C3>::iterator_category, std::forward_iterator_tag>);

// Bidirectional iterators: bidirectional_iterator_tag (/2.2.2).
using BidiV = ArchetypeView<BidiIter<int>>;
using C4 = rg::concat_view<BidiV, rg::ref_view<int[2]>>;
static_assert(rg::bidirectional_range<C4> && !rg::random_access_range<C4>);
static_assert(std::is_same_v<rg::iterator_t<C4>::iterator_category, std::bidirectional_iterator_tag>);

// iota's iterator_category is input_iterator_tag: /2.2.4.
using C5 = decltype(vw::concat(vw::iota(0, 2), vw::iota(3, 4)));
static_assert(std::is_same_v<rg::iterator_t<C5>::iterator_category, std::input_iterator_tag>);

// Input ranges: absent.
using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
static_assert(!has_iterator_category<rg::iterator_t<rg::concat_view<InV, rg::ref_view<int[2]>>>>);
