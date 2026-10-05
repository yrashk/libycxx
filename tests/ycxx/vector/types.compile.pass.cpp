// [vector.overview]: member types of vector; pointer / const_pointer come from
// allocator_traits<Allocator>; reference is T&; iterator and const_iterator are contiguous
// (vector is a contiguous container for T other than bool, [container.reqmts]/68);
// size_type is unsigned and can represent every non-negative difference_type;
// difference_type is the iterators' difference type ([container.reqmts]/8-9).
// [vector.overview]/2: push_front, prepend_range, pop_front and emplace_front are not
// provided.
// REQUIRES: exceptions
#include <vector>
#include <iterator>
#include <memory>
#include <type_traits>
#include "test_allocators.hpp"

template <class V, class T, class A>
constexpr bool check() {
  static_assert(std::is_same_v<typename V::value_type, T>);
  static_assert(std::is_same_v<typename V::allocator_type, A>);
  static_assert(std::is_same_v<typename V::pointer, typename std::allocator_traits<A>::pointer>);
  static_assert(std::is_same_v<typename V::const_pointer, typename std::allocator_traits<A>::const_pointer>);
  static_assert(std::is_same_v<typename V::reference, T&>);
  static_assert(std::is_same_v<typename V::const_reference, const T&>);
  static_assert(std::contiguous_iterator<typename V::iterator>);
  static_assert(std::contiguous_iterator<typename V::const_iterator>);
  static_assert(std::is_same_v<std::iter_reference_t<typename V::iterator>, T&>);
  static_assert(std::is_same_v<std::iter_reference_t<typename V::const_iterator>, const T&>);
  static_assert(std::is_same_v<std::iter_value_t<typename V::iterator>, T>);
  static_assert(std::is_convertible_v<typename V::iterator, typename V::const_iterator>);
  static_assert(!std::is_convertible_v<typename V::const_iterator, typename V::iterator>);
  static_assert(std::is_same_v<typename V::difference_type, std::iter_difference_t<typename V::iterator>>);
  static_assert(std::is_same_v<typename V::difference_type, std::iter_difference_t<typename V::const_iterator>>);
  static_assert(std::is_signed_v<typename V::difference_type>);
  static_assert(std::is_unsigned_v<typename V::size_type>);
  static_assert(sizeof(typename V::size_type) >= sizeof(typename V::difference_type));
  static_assert(std::is_same_v<typename V::reverse_iterator, std::reverse_iterator<typename V::iterator>>);
  static_assert(std::is_same_v<typename V::const_reverse_iterator,
                               std::reverse_iterator<typename V::const_iterator>>);
  static_assert(!requires(V& v) { v.push_front(std::declval<T>()); });
  static_assert(!requires(V& v) { v.pop_front(); });
  static_assert(!requires(V& v) { v.emplace_front(); });
  return true;
}

struct Elem {
  int x;
};
static_assert(check<std::vector<int>, int, std::allocator<int>>());
static_assert(check<std::vector<Elem>, Elem, std::allocator<Elem>>());
static_assert(check<std::vector<const char*>, const char*, std::allocator<const char*>>());
static_assert(check<std::vector<int, MinimalAlloc<int>>, int, MinimalAlloc<int>>());
static_assert(std::is_same_v<std::vector<int>, std::vector<int, std::allocator<int>>>);
