// Iterator concept conformance of the vector and basic_string iterators.
// [container.reqmts]/6-8,68: iterator and const_iterator of a contiguous container (vector<T>
// for T other than bool, basic_string) meet Cpp17RandomAccessIterator and model
// contiguous_iterator; iterator is mutable and converts to the constant const_iterator; the
// difference type is X::difference_type. [container.reqmts]/63: iterator and const_iterator
// can be mixed in ==, <, <=>, and -, so each is a sized_sentinel_for the other.
// [iterator.concepts] then gives: contiguous_iterator refines random_access_iterator,
// bidirectional_iterator, forward_iterator, input_iterator; forward_iterator requires
// sentinel_for<I, I> and regular/default_initializable; random_access_iterator requires
// totally_ordered and sized_sentinel_for<I, I>. [iterator.traits]: iterator_traits<I> gives
// the Cpp17 category (random access). [alg.req]: mutable contiguous iterators over a
// movable, totally ordered value type are permutable, mergeable and sortable.
#include <vector>
#include <string>
#include <concepts>
#include <functional>
#include <iterator>
#include <type_traits>

template <class X>
constexpr bool check() {
  using T = typename X::value_type;
  using It = typename X::iterator;
  using CIt = typename X::const_iterator;
  using D = typename X::difference_type;
  static_assert(std::contiguous_iterator<It> && std::contiguous_iterator<CIt>);
  static_assert(std::random_access_iterator<It> && std::random_access_iterator<CIt>);
  static_assert(std::bidirectional_iterator<It> && std::forward_iterator<It> && std::input_iterator<It>);
  static_assert(std::input_or_output_iterator<It> && std::weakly_incrementable<It> && std::incrementable<It>);
  static_assert(std::output_iterator<It, const T&> && std::output_iterator<It, T>);
  static_assert(!std::output_iterator<CIt, const T&>);
  static_assert(std::indirectly_readable<It> && std::indirectly_readable<CIt>);
  static_assert(std::regular<It> && std::regular<CIt>);
  static_assert(std::default_initializable<It> && std::copyable<It>);
  static_assert(std::totally_ordered<It> && std::totally_ordered<CIt>);
  static_assert(std::three_way_comparable<It, std::strong_ordering>);
  static_assert(std::sentinel_for<It, It> && std::sentinel_for<CIt, CIt>);
  static_assert(std::sized_sentinel_for<It, It> && std::sized_sentinel_for<CIt, CIt>);
  static_assert(std::sentinel_for<CIt, It> && std::sentinel_for<It, CIt>);
  static_assert(std::sized_sentinel_for<CIt, It> && std::sized_sentinel_for<It, CIt>);
  static_assert(std::equality_comparable_with<It, CIt>);
  static_assert(std::convertible_to<It, CIt>);

  static_assert(std::is_same_v<std::iter_value_t<It>, T>);
  static_assert(std::is_same_v<std::iter_reference_t<It>, T&>);
  static_assert(std::is_same_v<std::iter_reference_t<CIt>, const T&>);
  static_assert(std::is_same_v<std::iter_rvalue_reference_t<It>, T&&>);
  static_assert(std::is_same_v<std::iter_rvalue_reference_t<CIt>, const T&&>);
  static_assert(std::is_same_v<std::iter_common_reference_t<It>, T&>);
  static_assert(std::is_same_v<std::iter_difference_t<It>, D>);
  static_assert(std::is_same_v<std::iter_const_reference_t<It>, const T&>);
  static_assert(std::is_same_v<std::iter_const_reference_t<CIt>, const T&>);

  using Tr = std::iterator_traits<It>;
  static_assert(std::derived_from<typename Tr::iterator_category, std::random_access_iterator_tag>);
  static_assert(std::is_same_v<typename Tr::value_type, T>);
  static_assert(std::is_same_v<typename Tr::difference_type, D>);
  static_assert(std::is_same_v<typename Tr::reference, T&>);
  static_assert(std::derived_from<typename std::iterator_traits<CIt>::iterator_category, std::random_access_iterator_tag>);
  static_assert(std::is_same_v<typename std::iterator_traits<CIt>::reference, const T&>);

  static_assert(std::indirectly_writable<It, T> && std::indirectly_writable<It, T&&>);
  static_assert(!std::indirectly_writable<CIt, T>);
  static_assert(std::indirectly_movable_storable<It, It> && std::indirectly_copyable_storable<CIt, It>);
  static_assert(std::indirectly_swappable<It, It>);
  static_assert(!std::indirectly_swappable<CIt, CIt>);
  static_assert(std::permutable<It> && !std::permutable<CIt>);
  static_assert(std::sortable<It> && std::sortable<It, std::ranges::greater>);
  static_assert(std::mergeable<CIt, CIt, It>);
  static_assert(std::indirectly_comparable<CIt, It, std::ranges::equal_to>);
  static_assert(std::indirect_strict_weak_order<std::ranges::less, CIt>);

  // const_iterator is what basic_const_iterator / make_const_iterator would leave alone
  static_assert(std::is_same_v<std::const_iterator<CIt>, CIt>);
  static_assert(std::is_same_v<std::const_iterator<It>, std::basic_const_iterator<It>>);

  using RI = typename X::reverse_iterator;
  static_assert(std::random_access_iterator<RI> && !std::contiguous_iterator<RI>);
  static_assert(std::sized_sentinel_for<RI, RI>);
  static_assert(std::is_same_v<std::iter_reference_t<RI>, T&>);
  return true;
}

struct Big {
  int a[4];
  auto operator<=>(const Big&) const = default;
};

static_assert(check<std::vector<int>>());
static_assert(check<std::vector<double>>());
static_assert(check<std::vector<Big>>());
static_assert(check<std::vector<std::string>>());
static_assert(check<std::vector<std::vector<int>>>());
static_assert(check<std::vector<int*>>());
static_assert(check<std::string>());
static_assert(check<std::wstring>());
static_assert(check<std::u8string>());
static_assert(check<std::u16string>());
static_assert(check<std::u32string>());
