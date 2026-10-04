// Generic requirement checks extracted from tests/ycxx/containers/container_types.compile.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <concepts>
#include <iterator>
#include <limits>
#include <type_traits>
#include "container_values.hpp"

namespace reqs::container_types {

// AllocAware = false for containers that are not allocator-aware (inplace_vector).
template <class X, class T, bool AllocAware = true>
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
  if constexpr (AllocAware) static_assert(std::is_same_v<typename X::allocator_type::value_type, T>);
  return true;
}

}  // namespace reqs::container_types
