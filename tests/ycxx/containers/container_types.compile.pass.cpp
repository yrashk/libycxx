// [container.reqmts]/2-9: X::value_type is T; X::reference is T&; X::const_reference is const
// T&; X::iterator meets the forward iterator requirements with value type T and is convertible
// to X::const_iterator; X::const_iterator is a constant forward iterator with value type T;
// X::difference_type is a signed integer type identical to the difference type of iterator
// and const_iterator; X::size_type is an unsigned integer type that can represent any
// non-negative value of difference_type. [forward.iterators]/1: the reference type of a
// mutable forward iterator is T&, of a constant one const T&. [container.rev.reqmts]/2-3:
// reverse_iterator is reverse_iterator<iterator>, const_reverse_iterator is
// reverse_iterator<const_iterator>. [container.alloc.reqmts]/4-5: allocator_type, whose
// value_type is X::value_type.
// Written as a template to be instantiated for every container (vector and basic_string now,
// [basic.string.general]/2: basic_string is a contiguous container). vector<bool> is not a
// container in this sense (its reference is a proxy, [vector.bool.pspc]).
#include <vector>
#include <string>
#include <concepts>
#include <iterator>
#include <limits>
#include <type_traits>
#include "container_values.hpp"

template <class X, class T>
constexpr bool container_types() {
  using It = typename X::iterator;
  using CIt = typename X::const_iterator;
  using D = typename X::difference_type;
  using S = typename X::size_type;
  static_assert(std::is_same_v<typename X::value_type, T>);
  static_assert(std::is_same_v<typename X::reference, T&>);
  static_assert(std::is_same_v<typename X::const_reference, const T&>);

  static_assert(std::forward_iterator<It> && std::forward_iterator<CIt>);
  static_assert(std::derived_from<typename std::iterator_traits<It>::iterator_category, std::forward_iterator_tag>);
  static_assert(std::derived_from<typename std::iterator_traits<CIt>::iterator_category, std::forward_iterator_tag>);
  static_assert(std::is_same_v<std::iter_value_t<It>, T> && std::is_same_v<std::iter_value_t<CIt>, T>);
  static_assert(std::is_same_v<typename std::iterator_traits<It>::value_type, T>);
  static_assert(std::is_same_v<typename std::iterator_traits<CIt>::value_type, T>);
  static_assert(std::is_same_v<std::iter_reference_t<It>, T&>);
  static_assert(std::is_same_v<std::iter_reference_t<CIt>, const T&>);
  static_assert(std::is_same_v<typename std::iterator_traits<It>::reference, T&>);
  static_assert(std::is_same_v<typename std::iterator_traits<CIt>::reference, const T&>);
  static_assert(std::indirectly_writable<It, const T&>);
  static_assert(!std::indirectly_writable<CIt, const T&>);
  static_assert(std::is_convertible_v<It, CIt>);
  static_assert(std::is_convertible_v<const It&, CIt>);

  static_assert(std::is_integral_v<D> && std::is_signed_v<D>);
  static_assert(std::is_same_v<D, std::iter_difference_t<It>>);
  static_assert(std::is_same_v<D, std::iter_difference_t<CIt>>);
  static_assert(std::is_same_v<D, typename std::iterator_traits<It>::difference_type>);
  static_assert(std::is_integral_v<S> && std::is_unsigned_v<S>);
  static_assert(std::numeric_limits<S>::max() >= static_cast<std::make_unsigned_t<D>>(std::numeric_limits<D>::max()));

  static_assert(std::is_same_v<typename X::reverse_iterator, std::reverse_iterator<It>>);
  static_assert(std::is_same_v<typename X::const_reverse_iterator, std::reverse_iterator<CIt>>);
  static_assert(std::is_same_v<typename X::allocator_type::value_type, T>);
  return true;
}

struct Pod {
  int a;
  double b;
};

static_assert(container_types<std::vector<int>, int>());
static_assert(container_types<std::vector<const int*>, const int*>());
static_assert(container_types<std::vector<Pod>, Pod>());
static_assert(container_types<std::vector<Elem>, Elem>());
static_assert(container_types<std::vector<std::string>, std::string>());
static_assert(container_types<std::vector<std::vector<int>>, std::vector<int>>());
static_assert(container_types<std::string, char>());
static_assert(container_types<std::wstring, wchar_t>());
static_assert(container_types<std::u8string, char8_t>());
static_assert(container_types<std::u16string, char16_t>());
static_assert(container_types<std::u32string, char32_t>());
