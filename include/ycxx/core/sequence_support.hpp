// libycxx core: helpers shared by the sequence containers (vector, vector<bool>,
// inplace_vector): iterator-pair classification and distance, the from_range_t tag check, and
// in-place element rotation.
#pragma once

#include <ycxx/core/concepts.hpp>
#include <ycxx/core/container_base.hpp>

namespace ycxx::detail {

// The first parameter of the from_range_t constructors is a template parameter checked by
// this concept first: otherwise overload resolution for an unrelated call (vector(it, it))
// would check container-compatible-range on the other argument, which may perform ADL on a
// type that must not be completed.
template <class Tag>
concept from_range_tag = std::is_same_v<Tag, std::from_range_t>;

// A Cpp17ForwardIterator by category, or a C++20 forward iterator: may be traversed twice.
// The category is tested first: checking the C++20 concepts performs ADL for operator
// expressions on the iterator, which must not be needed for a plain Cpp17 iterator.
template <class It>
concept multipass_iterator =
    std::is_convertible_v<typename std::iterator_traits<It>::iterator_category, std::forward_iterator_tag> ||
    std::forward_iterator<It>;

// An iterator pair whose distance is last - first: random access by category, or (checked
// only otherwise) a sized sentinel.
template <class It>
concept subtractable_iterator =
    std::is_convertible_v<typename std::iterator_traits<It>::iterator_category, std::random_access_iterator_tag> ||
    std::sized_sentinel_for<It, It>;

// distance(first, last) for an iterator pair (which need not model sentinel_for).
template <class It>
constexpr auto iter_pair_distance(It first, It last) {
  if constexpr (subtractable_iterator<It>) {
    return last - first;
  } else {
    typename std::iterator_traits<It>::difference_type n = 0;
    for (; first != last; ++first)
      ++n;
    return n;
  }
}

// The address of the element an iterator refers to, for pointers and the library's own
// contiguous iterators (other iterator types are never inspected with the C++20 concepts, so
// iterators over incomplete-class pointers stay usable).
template <class It>
inline constexpr bool is_plain_contiguous = std::is_pointer_v<It>;
template <class T, class Owner, class Diff>
inline constexpr bool is_plain_contiguous<ycxx::adl_free::contiguous_iter<T, Owner, Diff>> = true;
template <class It>
constexpr auto plain_address(const It& it) noexcept {
  if constexpr (std::is_pointer_v<It>)
    return it;
  else
    return it.base();
}

// A temporary element for rotate_elements in containers without an allocator.
template <class T>
struct plain_temp {
  T v;
  constexpr explicit plain_temp(T&& x) : v(static_cast<T&&>(x)) {}
};

// Rotates [f, l) left so that *m becomes the first element, with move assignments and one
// temporary per cycle (Temp(ctx..., T&&), holding the element in .v). No unqualified calls.
template <class Temp, class T, class... Ctx>
constexpr void rotate_elements(T* f, T* m, T* l, Ctx&... ctx) {
  const std::ptrdiff_t n = l - f;
  const std::ptrdiff_t k = m - f;
  if (k == 0 || k == n)
    return;
  std::ptrdiff_t cycles = n;
  for (std::ptrdiff_t b = k; b != 0;) { // gcd(n, k) cycles
    const std::ptrdiff_t r = cycles % b;
    cycles = b;
    b = r;
  }
  for (std::ptrdiff_t i = 0; i != cycles; ++i) {
    Temp tmp(ctx..., static_cast<T&&>(f[i]));
    std::ptrdiff_t j = i;
    for (;;) {
      std::ptrdiff_t next = j + k;
      if (next >= n)
        next -= n;
      if (next == i)
        break;
      f[j] = static_cast<T&&>(f[next]);
      j = next;
    }
    f[j] = static_cast<T&&>(tmp.v);
  }
}

} // namespace ycxx::detail
