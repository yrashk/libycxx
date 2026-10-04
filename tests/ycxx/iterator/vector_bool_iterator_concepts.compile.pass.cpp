// [vector.bool.pspc]/1-3: vector<bool>'s iterators range over bits through the proxy class
// reference, so they are not contiguous ([iterator.concept.contiguous] requires
// is_lvalue_reference_v<iter_reference_t<I>>), but as the iterators of a sequence container
// with operator[] they are random access ([container.reqmts]/39-40: i <=> j is
// strong_ordering for a random access iterator; /63: iterator and const_iterator mix in
// comparisons and differences). reference has a const operator=(bool) const and swap
// friends, so iterator is indirectly_writable for bool ([iterator.concept.writable]),
// indirectly_swappable, and permutable / sortable ([alg.req.permutable],
// [alg.req.sortable]); const_iterator yields bool by value.
#include <vector>
#include <compare>
#include <concepts>
#include <functional>
#include <iterator>
#include <type_traits>

using VB = std::vector<bool>;
using It = VB::iterator;
using CIt = VB::const_iterator;
using R = VB::reference;

static_assert(std::random_access_iterator<It> && std::random_access_iterator<CIt>);
static_assert(!std::contiguous_iterator<It> && !std::contiguous_iterator<CIt>);
static_assert(std::regular<It> && std::totally_ordered<It>);
static_assert(std::is_same_v<std::compare_three_way_result_t<It>, std::strong_ordering>);
static_assert(std::sized_sentinel_for<It, It> && std::sized_sentinel_for<CIt, CIt>);
static_assert(std::sized_sentinel_for<CIt, It> && std::sized_sentinel_for<It, CIt>);
static_assert(std::convertible_to<It, CIt>);
static_assert(std::is_same_v<std::iter_value_t<It>, bool> && std::is_same_v<std::iter_value_t<CIt>, bool>);
static_assert(std::is_same_v<std::iter_reference_t<It>, R>);
static_assert(std::is_same_v<std::iter_reference_t<CIt>, bool>);
static_assert(std::is_same_v<std::iter_difference_t<It>, VB::difference_type>);
static_assert(std::indirectly_writable<It, bool>);
static_assert(std::output_iterator<It, bool>);
static_assert(!std::indirectly_writable<CIt, bool>);
static_assert(std::indirectly_swappable<It, It>);
static_assert(std::permutable<It>);
static_assert(std::sortable<It>);
static_assert(std::indirect_strict_weak_order<std::ranges::less, CIt>);
static_assert(std::is_same_v<std::iterator_traits<It>::value_type, bool>);
static_assert(std::is_same_v<std::iterator_traits<It>::difference_type, VB::difference_type>);
static_assert(std::random_access_iterator<VB::reverse_iterator>);
