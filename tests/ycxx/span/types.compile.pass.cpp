// [span.syn], [span.overview]: dynamic_extent, member types, extent, enable_view /
// enable_borrowed_range, "span<ElementType, Extent> is a trivially copyable type".
// [span.iterators]/1: iterator "models contiguous_iterator ... whose value type is value_type
// and whose reference type is reference"; const_iterator = std::const_iterator<iterator>;
// reverse_iterator = std::reverse_iterator<iterator>;
// const_reverse_iterator = std::const_iterator<reverse_iterator>.
#include <span>
#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>

static_assert(std::is_same_v<decltype(std::dynamic_extent), const std::size_t>);
static_assert(std::dynamic_extent == std::numeric_limits<std::size_t>::max());

template <class T, std::size_t E>
constexpr bool check() {
  using S = std::span<T, E>;
  static_assert(std::is_same_v<typename S::element_type, T>);
  static_assert(std::is_same_v<typename S::value_type, std::remove_cv_t<T>>);
  static_assert(std::is_same_v<typename S::size_type, std::size_t>);
  static_assert(std::is_same_v<typename S::difference_type, std::ptrdiff_t>);
  static_assert(std::is_same_v<typename S::pointer, T*>);
  static_assert(std::is_same_v<typename S::const_pointer, const T*>);
  static_assert(std::is_same_v<typename S::reference, T&>);
  static_assert(std::is_same_v<typename S::const_reference, const T&>);
  static_assert(std::is_same_v<typename S::const_iterator, std::const_iterator<typename S::iterator>>);
  static_assert(std::is_same_v<typename S::reverse_iterator, std::reverse_iterator<typename S::iterator>>);
  static_assert(
      std::is_same_v<typename S::const_reverse_iterator, std::const_iterator<typename S::reverse_iterator>>);
  static_assert(std::contiguous_iterator<typename S::iterator>);
  static_assert(std::is_same_v<std::iter_value_t<typename S::iterator>, std::remove_cv_t<T>>);
  static_assert(std::is_same_v<std::iter_reference_t<typename S::iterator>, T&>);
  if constexpr (!std::is_volatile_v<T>)
    static_assert(std::is_same_v<std::iter_reference_t<typename S::const_iterator>, const T&>);
  static_assert(std::is_same_v<decltype(S::extent), const std::size_t>);
  static_assert(S::extent == E);
  static_assert(std::is_trivially_copyable_v<S>);
  static_assert(std::is_nothrow_copy_constructible_v<S>);
  static_assert(std::is_nothrow_copy_assignable_v<S>);
  static_assert(std::ranges::enable_view<S>);
  static_assert(std::ranges::enable_borrowed_range<S>);
  return true;
}
static_assert(check<int, std::dynamic_extent>());
static_assert(check<int, 3>());
static_assert(check<const int, 0>());
static_assert(check<volatile double, 2>());
static_assert(check<const volatile char, std::dynamic_extent>());
static_assert(std::span<int>::extent == std::dynamic_extent);  // default template argument
