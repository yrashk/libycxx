// [vector.overview]/1-2: vector is a contiguous container; with [range.access] and
// [range.req] it models contiguous_range, sized_range and common_range; it is not a view
// (copying is linear, enable_view is not specialized) and not a borrowed_range;
// viewable_range holds for lvalues (ref_view) and non-const rvalues (owning_view,
// [range.owning.view]) but not for const rvalues (not movable). range_reference_t is T& and
// T is an output range of T for a non-const vector only; a const vector is a constant_range
// ([range.refinements]). vector<bool> ([vector.bool.pspc]) is random access but not
// contiguous, and its range_reference_t is the proxy reference.
#include <vector>
#include <ranges>
#include <type_traits>

template <class T>
constexpr bool check() {
  using V = std::vector<T>;
  static_assert(std::ranges::contiguous_range<V> && std::ranges::contiguous_range<const V>);
  static_assert(std::ranges::random_access_range<V> && std::ranges::sized_range<V>);
  static_assert(std::ranges::common_range<V> && std::ranges::common_range<const V>);
  static_assert(!std::ranges::view<V> && !std::ranges::enable_view<V>);
  static_assert(!std::ranges::borrowed_range<V> && !std::ranges::borrowed_range<V&&>);
  static_assert(std::ranges::borrowed_range<V&>);
  static_assert(std::ranges::viewable_range<V&> && std::ranges::viewable_range<const V&>);
  static_assert(std::ranges::viewable_range<V>);
  static_assert(!std::ranges::viewable_range<const V>);
  static_assert(std::is_same_v<std::ranges::range_value_t<V>, T>);
  static_assert(std::is_same_v<std::ranges::range_reference_t<V>, T&>);
  static_assert(std::is_same_v<std::ranges::range_reference_t<const V>, const T&>);
  static_assert(std::is_same_v<std::ranges::range_size_t<V>, typename V::size_type>);
  static_assert(std::is_same_v<std::ranges::range_difference_t<V>, typename V::difference_type>);
  static_assert(std::is_same_v<std::ranges::iterator_t<V>, typename V::iterator>);
  static_assert(std::is_same_v<std::ranges::iterator_t<const V>, typename V::const_iterator>);
  static_assert(std::is_same_v<std::ranges::sentinel_t<V>, typename V::iterator>);
  static_assert(std::ranges::output_range<V&, T> && !std::ranges::output_range<const V&, T>);
  static_assert(std::ranges::constant_range<const V> && !std::ranges::constant_range<V>);
  static_assert(std::is_same_v<decltype(std::views::all(std::declval<V&>())), std::ranges::ref_view<V>>);
  static_assert(std::is_same_v<decltype(std::views::all(std::declval<V>())), std::ranges::owning_view<V>>);
  static_assert(std::is_same_v<decltype(std::ranges::data(std::declval<V&>())), T*>);
  return true;
}

struct S {
  int x;
};
static_assert(check<int>());
static_assert(check<S>());
static_assert(check<std::vector<int>>());

using VB = std::vector<bool>;
static_assert(std::ranges::random_access_range<VB> && std::ranges::sized_range<VB>);
static_assert(std::ranges::common_range<VB>);
static_assert(!std::ranges::contiguous_range<VB>);
static_assert(!std::ranges::view<VB> && !std::ranges::borrowed_range<VB>);
static_assert(std::is_same_v<std::ranges::range_value_t<VB>, bool>);
static_assert(std::is_same_v<std::ranges::range_reference_t<VB>, VB::reference>);
static_assert(std::is_same_v<std::ranges::range_reference_t<const VB>, bool>);
static_assert(std::ranges::output_range<VB&, bool>);
