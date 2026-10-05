// libycxx core: pieces shared by the containers ([container.reqmts]): the "qualifies as an
// allocator / input iterator" tests, container-compatible-range, and the contiguous iterator
// class used by contiguous containers.
#pragma once

#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/range_access.hpp>
#include <ycxx/core/ranges_base.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// [container.reqmts]: a type qualifies as an allocator if A::value_type is a type and
// declval<A&>().allocate(size_t{}) is well-formed.
template <class A>
concept qualifies_as_allocator = requires(A& a) {
  typename A::value_type;
  a.allocate(std::size_t{});
};

// allocator_traits<A>::size_type, a substitution failure when A does not qualify as an
// allocator: an implicit deduction guide from a constructor such as
// vector(size_type, const T&, const Allocator&) then drops out instead of instantiating X<T, A> ([container.reqmts]/69, [container.requirements.general]).
template <class A>
  requires qualifies_as_allocator<A>
using alloc_size_t = typename std::allocator_traits<A>::size_type;

// [container.reqmts]: integral types never qualify as input iterators; libycxx also requires
// an iterator_category derived from input_iterator_tag.
template <class I>
concept qualifies_as_input_iterator = !std::is_integral_v<I> && requires {
  typename std::iterator_traits<I>::iterator_category;
} && std::is_convertible_v<typename std::iterator_traits<I>::iterator_category, std::input_iterator_tag>;

// [container.intro.reqmts]
template <class R, class T>
concept container_compatible_range =
    std::ranges::input_range<R> && std::convertible_to<std::ranges::range_reference_t<R>, T>;

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// The iterator of a contiguous container: a wrapped T* (T possibly const). Owner makes the
// iterators of different containers distinct types; Diff is the container's difference_type.
// A class rather than a raw pointer so that 0 or nullptr never converts to an iterator (which
// would make overloads such as basic_string::insert(size_type, ...) /
// insert(const_iterator, ...) ambiguous). Comparisons are hidden friends taking two iterators of
// the same type; an iterator converts to the matching const iterator, so mixed comparisons work.
template <class T, class Owner, class Diff>
class contiguous_iter {
  T* p_ = nullptr;

public:
  using iterator_concept = std::contiguous_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::remove_cv_t<T>;
  using difference_type = Diff;
  using pointer = T*;
  using reference = T&;

  constexpr contiguous_iter() noexcept = default;
  constexpr explicit contiguous_iter(T* p) noexcept : p_(p) {}
  template <class U>
    requires std::is_same_v<const U, T> && (!std::is_same_v<U, T>)
  constexpr contiguous_iter(const contiguous_iter<U, Owner, Diff>& o) noexcept : p_(o.base()) {}

  constexpr T* base() const noexcept { return p_; }

  constexpr reference operator*() const noexcept { return *p_; }
  constexpr pointer operator->() const noexcept { return p_; }
  constexpr reference operator[](difference_type n) const noexcept { return p_[n]; }

  constexpr contiguous_iter& operator++() noexcept {
    ++p_;
    return *this;
  }
  constexpr contiguous_iter operator++(int) noexcept { return contiguous_iter(p_++); }
  constexpr contiguous_iter& operator--() noexcept {
    --p_;
    return *this;
  }
  constexpr contiguous_iter operator--(int) noexcept { return contiguous_iter(p_--); }
  constexpr contiguous_iter& operator+=(difference_type n) noexcept {
    p_ += n;
    return *this;
  }
  constexpr contiguous_iter& operator-=(difference_type n) noexcept {
    p_ -= n;
    return *this;
  }

  friend constexpr contiguous_iter operator+(contiguous_iter i, difference_type n) noexcept { return i += n; }
  friend constexpr contiguous_iter operator+(difference_type n, contiguous_iter i) noexcept { return i += n; }
  friend constexpr contiguous_iter operator-(contiguous_iter i, difference_type n) noexcept { return i -= n; }
  friend constexpr difference_type operator-(const contiguous_iter& a, const contiguous_iter& b) noexcept {
    return static_cast<difference_type>(a.p_ - b.p_);
  }
  friend constexpr bool operator==(const contiguous_iter& a, const contiguous_iter& b) noexcept {
    return a.p_ == b.p_;
  }
  friend constexpr std::strong_ordering operator<=>(const contiguous_iter& a, const contiguous_iter& b) noexcept {
    return a.p_ <=> b.p_;
  }
};

}} // namespace ycxx::adl_free
