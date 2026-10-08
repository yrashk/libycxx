// libycxx core: pieces shared by the containers ([container.reqmts]): the "qualifies as an
// allocator / input iterator" tests, container-compatible-range, and the contiguous iterator
// class used by contiguous containers.
#pragma once

#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/range_access.hpp>
#include <ycxx/core/ranges_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// [container.reqmts]: a type qualifies as an allocator if A::value_type is a type and
// declval<A&>().allocate(size_t{}) is well-formed.
template <class _Ap>
concept __qualifies_as_allocator = requires(_Ap& a) {
  typename _Ap::value_type;
  a.allocate(std::size_t{});
};

// allocator_traits<A>::size_type, a substitution failure when A does not qualify as an
// allocator: an implicit deduction guide from a constructor such as
// vector(size_type, const T&, const Allocator&) then drops out instead of instantiating X<T, A> ([container.reqmts]/69, [container.requirements.general]).
template <class _Ap>
  requires __qualifies_as_allocator<_Ap>
using __alloc_size_t = typename std::allocator_traits<_Ap>::size_type;

// [container.reqmts]: integral types never qualify as input iterators; libycxx also requires
// an iterator_category derived from input_iterator_tag.
template <class _Ip>
concept __qualifies_as_input_iterator = !std::is_integral_v<_Ip> && requires {
  typename std::iterator_traits<_Ip>::iterator_category;
} && std::is_convertible_v<typename std::iterator_traits<_Ip>::iterator_category, std::input_iterator_tag>;

// A copy of the allocator a container is constructed with. An empty allocator whose copy and
// default constructors are trivial has no state to copy, so the copy is value-initialized
// instead, which is the same value, without reading a. GCC 16 crashes (ICE in
// cxx_eval_indirect_ref) when it constant-folds an array of aggregates whose members are
// containers of different allocator types constructed with the default allocator argument
// (struct { std::string in; std::vector<std::string> out; } a[] = {{"", {""}}};): the
// default argument it binds is the other member's allocator temporary, and reading it through
// the reference crashes.
// noexcept only when the copy is: a (non-conforming) throwing allocator copy propagates from the
// constructors that are not noexcept themselves (libc++'s vector ctor_exceptions tests).
template <class _Ap>
constexpr _Ap __alloc_copy(const _Ap& __a) noexcept(std::is_nothrow_copy_constructible_v<_Ap>) {
  if constexpr (std::is_empty_v<_Ap> && std::is_trivially_copy_constructible_v<_Ap> &&
                std::is_trivially_default_constructible_v<_Ap>)
    return _Ap();
  else
    return __a;
}

// [container.intro.reqmts]
template <class _Rp, class _Tp>
concept __container_compatible_range =
    std::ranges::input_range<_Rp> && std::convertible_to<std::ranges::range_reference_t<_Rp>, _Tp>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {

// The iterator of a contiguous container: a wrapped T* (T possibly const). Owner makes the
// iterators of different containers distinct types; Diff is the container's difference_type.
// A class rather than a raw pointer so that 0 or nullptr never converts to an iterator (which
// would make overloads such as basic_string::insert(size_type, ...) /
// insert(const_iterator, ...) ambiguous). Comparisons are hidden friends taking two iterators of
// the same type; an iterator converts to the matching const iterator, so mixed comparisons work.
template <class _Tp, class _Owner, class _Diff>
class __contiguous_iter {
  _Tp* __p_ = nullptr;

public:
  using iterator_concept = std::contiguous_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::remove_cv_t<_Tp>;
  using difference_type = _Diff;
  using pointer = _Tp*;
  using reference = _Tp&;

  constexpr __contiguous_iter() noexcept = default;
  constexpr explicit __contiguous_iter(_Tp* p) noexcept : __p_(p) {}
  template <class _Up>
    requires std::is_same_v<const _Up, _Tp> && (!std::is_same_v<_Up, _Tp>)
  constexpr __contiguous_iter(const __contiguous_iter<_Up, _Owner, _Diff>& __o) noexcept : __p_(__o.base()) {}

  constexpr _Tp* base() const noexcept { return __p_; }

  constexpr reference operator*() const noexcept { return *__p_; }
  constexpr pointer operator->() const noexcept { return __p_; }
  constexpr reference operator[](difference_type n) const noexcept { return __p_[n]; }

  constexpr __contiguous_iter& operator++() noexcept {
    ++__p_;
    return *this;
  }
  constexpr __contiguous_iter operator++(int) noexcept { return __contiguous_iter(__p_++); }
  constexpr __contiguous_iter& operator--() noexcept {
    --__p_;
    return *this;
  }
  constexpr __contiguous_iter operator--(int) noexcept { return __contiguous_iter(__p_--); }
  constexpr __contiguous_iter& operator+=(difference_type n) noexcept {
    __p_ += n;
    return *this;
  }
  constexpr __contiguous_iter& operator-=(difference_type n) noexcept {
    __p_ -= n;
    return *this;
  }

  friend constexpr __contiguous_iter operator+(__contiguous_iter i, difference_type n) noexcept { return i += n; }
  friend constexpr __contiguous_iter operator+(difference_type n, __contiguous_iter i) noexcept { return i += n; }
  friend constexpr __contiguous_iter operator-(__contiguous_iter i, difference_type n) noexcept { return i -= n; }
  friend constexpr difference_type operator-(const __contiguous_iter& a, const __contiguous_iter& b) noexcept {
    return static_cast<difference_type>(a.__p_ - b.__p_);
  }
  friend constexpr bool operator==(const __contiguous_iter& a, const __contiguous_iter& b) noexcept {
    return a.__p_ == b.__p_;
  }
  friend constexpr std::strong_ordering operator<=>(const __contiguous_iter& a, const __contiguous_iter& b) noexcept {
    return a.__p_ <=> b.__p_;
  }
};

}} // namespace __ycxx::__adl_free
