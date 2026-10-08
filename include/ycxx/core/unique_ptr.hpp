// libycxx core: default_delete, unique_ptr, make_unique[_for_overwrite], the unique_ptr
// comparisons and hash support ([unique.ptr], [util.smartptr.hash]).
//
// The deleter is stored with [[no_unique_address]], so unique_ptr<T> with an empty deleter is
// the size of a pointer. A reference deleter (D = A&) is stored as a reference member.
// operator<< ([unique.ptr.io]) is written against the declaration of basic_ostream.
#pragma once

#include <ycxx/core/iosfwd.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/new.hpp>
#include <ycxx/core/error.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [unique.ptr.dltr.dflt]
template <class _Tp>
struct default_delete {
  constexpr default_delete() noexcept = default;
  template <class _Up>
    requires is_convertible_v<_Up*, _Tp*>
  constexpr default_delete(const default_delete<_Up>&) noexcept {}

  constexpr void operator()(_Tp* ptr) const {
    static_assert(!is_void_v<_Tp>, "std::default_delete: cannot delete a pointer to void");
    static_assert(requires { sizeof(_Tp); }, "std::default_delete: T must be a complete type");
    delete ptr;
  }
};

// [unique.ptr.dltr.dflt1]
template <class _Tp>
struct default_delete<_Tp[]> {
  constexpr default_delete() noexcept = default;
  template <class _Up>
    requires is_convertible_v<_Up (*)[], _Tp (*)[]>
  constexpr default_delete(const default_delete<_Up[]>&) noexcept {}

  template <class _Up>
    requires is_convertible_v<_Up (*)[], _Tp (*)[]>
  constexpr void operator()(_Up* ptr) const {
    static_assert(requires { sizeof(_Up); }, "std::default_delete<T[]>: U must be a complete type");
    delete[] ptr;
  }
};

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// unique_ptr<T, D>::pointer ([unique.ptr.single.general]/4).
template <class _Tp, class _Dp>
struct __uptr_pointer {
  using type = _Tp*;
};
template <class _Tp, class _Dp>
  requires requires { typename std::remove_reference_t<_Dp>::pointer; }
struct __uptr_pointer<_Tp, _Dp> {
  using type = typename std::remove_reference_t<_Dp>::pointer;
};

// "either D is a reference type and E is the same type as D, or D is not a reference type and
// E is implicitly convertible to D" ([unique.ptr.single.ctor]/19.3).
template <class _Dp, class _Ep>
concept __uptr_deleter_from = (std::is_reference_v<_Dp> ? std::is_same_v<_Ep, _Dp> : std::is_convertible_v<_Ep, _Dp>);

// V(*)[] is convertible to element_type(*)[] for a pointer type U = V*; the array forms'
// constructor and reset constraints ([unique.ptr.runtime.ctor]/2).
template <class _Up, class _Pointer, class _Elem>
concept __uptr_array_ptr = std::is_same_v<_Up, _Pointer> ||
                         (std::is_same_v<_Pointer, _Elem*> && std::is_pointer_v<_Up> &&
                          std::is_convertible_v<std::remove_pointer_t<_Up> (*)[], _Elem (*)[]>);

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [unique.ptr.single]
template <class _Tp, class _Dp = default_delete<_Tp>>
class unique_ptr {
public:
  using pointer = typename __ycxx::__detail::__uptr_pointer<_Tp, _Dp>::type;
  using element_type = _Tp;
  using deleter_type = _Dp;

private:
  template <class, class>
  friend class unique_ptr;

  pointer __p_;
  [[no_unique_address]] _Dp __d_;

public:
  // [unique.ptr.single.ctor]
  constexpr unique_ptr() noexcept
    requires(!is_pointer_v<_Dp> && is_default_constructible_v<_Dp>)
      : __p_(), __d_() {}
  constexpr unique_ptr(nullptr_t) noexcept
    requires(!is_pointer_v<_Dp> && is_default_constructible_v<_Dp>)
      : __p_(), __d_() {}
  constexpr explicit unique_ptr(type_identity_t<pointer> p) noexcept
    requires(!is_pointer_v<_Dp> && is_default_constructible_v<_Dp>)
      : __p_(p), __d_() {}
  constexpr unique_ptr(type_identity_t<pointer> p, const _Dp& d) noexcept
    requires is_constructible_v<_Dp, const _Dp&>
      : __p_(p), __d_(d) {}
  constexpr unique_ptr(type_identity_t<pointer> p, remove_reference_t<_Dp>&& d) noexcept
    requires(!is_reference_v<_Dp> && is_constructible_v<_Dp, _Dp &&>)
      : __p_(p), __d_(static_cast<_Dp&&>(d)) {}
  unique_ptr(type_identity_t<pointer>, remove_reference_t<_Dp>&&)
    requires is_reference_v<_Dp>
  = delete;
  constexpr unique_ptr(unique_ptr&& __u) noexcept
    requires is_move_constructible_v<_Dp>
      : __p_(__u.release()), __d_(static_cast<_Dp&&>(__u.__d_)) {}
  template <class _Up, class _Ep>
    requires(is_convertible_v<typename unique_ptr<_Up, _Ep>::pointer, pointer> && !is_array_v<_Up> &&
             __ycxx::__detail::__uptr_deleter_from<_Dp, _Ep>)
  constexpr unique_ptr(unique_ptr<_Up, _Ep>&& __u) noexcept : __p_(__u.release()), __d_(static_cast<_Ep&&>(__u.__d_)) {}

  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  // [unique.ptr.single.dtor]
  constexpr ~unique_ptr() {
    if (__p_ != nullptr)
      __d_(__p_);
  }

  // [unique.ptr.single.asgn]
  constexpr unique_ptr& operator=(unique_ptr&& __u) noexcept
    requires is_move_assignable_v<_Dp>
  {
    reset(__u.release());
    __d_ = static_cast<_Dp&&>(__u.__d_);
    return *this;
  }
  template <class _Up, class _Ep>
    requires(is_convertible_v<typename unique_ptr<_Up, _Ep>::pointer, pointer> && !is_array_v<_Up> &&
             is_assignable_v<_Dp&, _Ep &&>)
  constexpr unique_ptr& operator=(unique_ptr<_Up, _Ep>&& __u) noexcept {
    reset(__u.release());
    __d_ = static_cast<_Ep&&>(__u.__d_);
    return *this;
  }
  constexpr unique_ptr& operator=(nullptr_t) noexcept {
    reset();
    return *this;
  }

  // [unique.ptr.single.observers]
  constexpr add_lvalue_reference_t<_Tp> operator*() const noexcept(noexcept(*declval<pointer>()))
    requires requires { *declval<pointer>(); }
  {
    static_assert(!reference_converts_from_temporary_v<add_lvalue_reference_t<_Tp>, decltype(*declval<pointer>())>,
                  "std::unique_ptr::operator*: would bind a reference to a temporary");
    __ycxx::__detail::__precondition(__p_ != nullptr, "std::unique_ptr::operator*: null pointer");
    return *__p_;
  }
  constexpr pointer operator->() const noexcept {
    __ycxx::__detail::__precondition(__p_ != nullptr, "std::unique_ptr::operator->: null pointer");
    return __p_;
  }
  constexpr pointer get() const noexcept { return __p_; }
  constexpr deleter_type& get_deleter() noexcept { return __d_; }
  constexpr const deleter_type& get_deleter() const noexcept { return __d_; }
  constexpr explicit operator bool() const noexcept { return __p_ != nullptr; }

  // [unique.ptr.single.modifiers]
  constexpr pointer release() noexcept {
    pointer __old = __p_;
    __p_ = pointer();
    return __old;
  }
  constexpr void reset(pointer p = pointer()) noexcept {
    pointer __old = __p_;
    __p_ = p;
    if (__old != nullptr)
      __d_(__old);
  }
  constexpr void swap(unique_ptr& __u) noexcept {
    __ycxx::__detail::__swap_adl::__do_swap(__p_, __u.__p_);
    __ycxx::__detail::__swap_adl::__do_swap(__d_, __u.__d_);
  }
};

// [unique.ptr.runtime]
template <class _Tp, class _Dp>
class unique_ptr<_Tp[], _Dp> {
public:
  using pointer = typename __ycxx::__detail::__uptr_pointer<_Tp, _Dp>::type;
  using element_type = _Tp;
  using deleter_type = _Dp;

private:
  template <class, class>
  friend class unique_ptr;

  // Constraints of the converting constructor and assignment ([unique.ptr.runtime.ctor]/6).
  template <class _Up, class _Ep>
  static constexpr bool __convertible_from =
      is_array_v<_Up> && is_same_v<pointer, _Tp*> &&
      is_same_v<typename unique_ptr<_Up, _Ep>::pointer, typename unique_ptr<_Up, _Ep>::element_type*> &&
      is_convertible_v<typename unique_ptr<_Up, _Ep>::element_type (*)[], _Tp (*)[]>;

  pointer __p_;
  [[no_unique_address]] _Dp __d_;

public:
  // [unique.ptr.runtime.ctor]
  constexpr unique_ptr() noexcept
    requires(!is_pointer_v<_Dp> && is_default_constructible_v<_Dp>)
      : __p_(), __d_() {}
  constexpr unique_ptr(nullptr_t) noexcept
    requires(!is_pointer_v<_Dp> && is_default_constructible_v<_Dp>)
      : __p_(), __d_() {}
  template <class _Up>
    requires(!is_pointer_v<_Dp> && is_default_constructible_v<_Dp> && __ycxx::__detail::__uptr_array_ptr<_Up, pointer, _Tp>)
  constexpr explicit unique_ptr(_Up p) noexcept : __p_(p), __d_() {}
  template <class _Up>
    requires((__ycxx::__detail::__uptr_array_ptr<_Up, pointer, _Tp> || is_same_v<_Up, nullptr_t>) &&
             is_constructible_v<_Dp, const _Dp&>)
  constexpr unique_ptr(_Up p, const _Dp& d) noexcept : __p_(p), __d_(d) {}
  template <class _Up>
    requires((__ycxx::__detail::__uptr_array_ptr<_Up, pointer, _Tp> || is_same_v<_Up, nullptr_t>) && !is_reference_v<_Dp> &&
             is_constructible_v<_Dp, _Dp &&>)
  constexpr unique_ptr(_Up p, remove_reference_t<_Dp>&& d) noexcept : __p_(p), __d_(static_cast<_Dp&&>(d)) {}
  template <class _Up>
    requires((__ycxx::__detail::__uptr_array_ptr<_Up, pointer, _Tp> || is_same_v<_Up, nullptr_t>) && is_reference_v<_Dp>)
  unique_ptr(_Up, remove_reference_t<_Dp>&&) = delete;
  constexpr unique_ptr(unique_ptr&& __u) noexcept
    requires is_move_constructible_v<_Dp>
      : __p_(__u.release()), __d_(static_cast<_Dp&&>(__u.__d_)) {}
  template <class _Up, class _Ep>
    requires(__convertible_from<_Up, _Ep> && __ycxx::__detail::__uptr_deleter_from<_Dp, _Ep>)
  constexpr unique_ptr(unique_ptr<_Up, _Ep>&& __u) noexcept : __p_(__u.release()), __d_(static_cast<_Ep&&>(__u.__d_)) {}

  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  constexpr ~unique_ptr() {
    if (__p_ != nullptr)
      __d_(__p_);
  }

  // [unique.ptr.runtime.asgn]
  constexpr unique_ptr& operator=(unique_ptr&& __u) noexcept
    requires is_move_assignable_v<_Dp>
  {
    reset(__u.release());
    __d_ = static_cast<_Dp&&>(__u.__d_);
    return *this;
  }
  template <class _Up, class _Ep>
    requires(__convertible_from<_Up, _Ep> && is_assignable_v<_Dp&, _Ep &&>)
  constexpr unique_ptr& operator=(unique_ptr<_Up, _Ep>&& __u) noexcept {
    reset(__u.release());
    __d_ = static_cast<_Ep&&>(__u.__d_);
    return *this;
  }
  constexpr unique_ptr& operator=(nullptr_t) noexcept {
    reset();
    return *this;
  }

  // [unique.ptr.runtime.observers]
  constexpr _Tp& operator[](size_t i) const {
    __ycxx::__detail::__precondition(__p_ != nullptr, "std::unique_ptr<T[]>::operator[]: null pointer");
    return __p_[i];
  }
  constexpr pointer get() const noexcept { return __p_; }
  constexpr deleter_type& get_deleter() noexcept { return __d_; }
  constexpr const deleter_type& get_deleter() const noexcept { return __d_; }
  constexpr explicit operator bool() const noexcept { return __p_ != nullptr; }

  // [unique.ptr.runtime.modifiers]
  constexpr pointer release() noexcept {
    pointer __old = __p_;
    __p_ = pointer();
    return __old;
  }
  template <class _Up>
    requires __ycxx::__detail::__uptr_array_ptr<_Up, pointer, _Tp>
  constexpr void reset(_Up p) noexcept {
    pointer __old = __p_;
    __p_ = p;
    if (__old != nullptr)
      __d_(__old);
  }
  constexpr void reset(nullptr_t = nullptr) noexcept { reset(pointer()); }
  constexpr void swap(unique_ptr& __u) noexcept {
    __ycxx::__detail::__swap_adl::__do_swap(__p_, __u.__p_);
    __ycxx::__detail::__swap_adl::__do_swap(__d_, __u.__d_);
  }
};

// [unique.ptr.create]
template <class _Tp, class... _Args>
  requires(!is_array_v<_Tp>)
constexpr unique_ptr<_Tp> make_unique(_Args&&... __args) {
  return unique_ptr<_Tp>(new _Tp(static_cast<_Args&&>(__args)...));
}
template <class _Tp>
  requires is_unbounded_array_v<_Tp>
constexpr unique_ptr<_Tp> make_unique(size_t n) {
  return unique_ptr<_Tp>(new remove_extent_t<_Tp>[n]());
}
template <class _Tp, class... _Args>
  requires is_bounded_array_v<_Tp>
void make_unique(_Args&&...) = delete;

template <class _Tp>
  requires(!is_array_v<_Tp>)
constexpr unique_ptr<_Tp> make_unique_for_overwrite() {
  return unique_ptr<_Tp>(new _Tp);
}
template <class _Tp>
  requires is_unbounded_array_v<_Tp>
constexpr unique_ptr<_Tp> make_unique_for_overwrite(size_t n) {
  return unique_ptr<_Tp>(new remove_extent_t<_Tp>[n]);
}
template <class _Tp, class... _Args>
  requires is_bounded_array_v<_Tp>
void make_unique_for_overwrite(_Args&&...) = delete;

// [unique.ptr.special]
template <class _Tp, class _Dp>
  requires is_swappable_v<_Dp>
constexpr void swap(unique_ptr<_Tp, _Dp>& __x, unique_ptr<_Tp, _Dp>& y) noexcept {
  __x.swap(y);
}

template <class _T1, class _D1, class _T2, class _D2>
constexpr bool operator==(const unique_ptr<_T1, _D1>& __x, const unique_ptr<_T2, _D2>& y) {
  return __x.get() == y.get();
}
template <class _T1, class _D1, class _T2, class _D2>
constexpr bool operator<(const unique_ptr<_T1, _D1>& __x, const unique_ptr<_T2, _D2>& y) {
  using _CT = common_type_t<typename unique_ptr<_T1, _D1>::pointer, typename unique_ptr<_T2, _D2>::pointer>;
  static_assert(is_convertible_v<typename unique_ptr<_T1, _D1>::pointer, _CT> &&
                    is_convertible_v<typename unique_ptr<_T2, _D2>::pointer, _CT>,
                "std::unique_ptr operator<: pointers must convert to their common type");
  return less<_CT>()(__x.get(), y.get());
}
template <class _T1, class _D1, class _T2, class _D2>
constexpr bool operator>(const unique_ptr<_T1, _D1>& __x, const unique_ptr<_T2, _D2>& y) {
  return y < __x;
}
template <class _T1, class _D1, class _T2, class _D2>
constexpr bool operator<=(const unique_ptr<_T1, _D1>& __x, const unique_ptr<_T2, _D2>& y) {
  return !(y < __x);
}
template <class _T1, class _D1, class _T2, class _D2>
constexpr bool operator>=(const unique_ptr<_T1, _D1>& __x, const unique_ptr<_T2, _D2>& y) {
  return !(__x < y);
}
template <class _T1, class _D1, class _T2, class _D2>
  requires three_way_comparable_with<typename unique_ptr<_T1, _D1>::pointer, typename unique_ptr<_T2, _D2>::pointer>
constexpr compare_three_way_result_t<typename unique_ptr<_T1, _D1>::pointer, typename unique_ptr<_T2, _D2>::pointer>
operator<=>(const unique_ptr<_T1, _D1>& __x, const unique_ptr<_T2, _D2>& y) {
  return compare_three_way()(__x.get(), y.get());
}

template <class _Tp, class _Dp>
constexpr bool operator==(const unique_ptr<_Tp, _Dp>& __x, nullptr_t) noexcept {
  return !__x;
}
template <class _Tp, class _Dp>
constexpr bool operator<(const unique_ptr<_Tp, _Dp>& __x, nullptr_t) {
  return less<typename unique_ptr<_Tp, _Dp>::pointer>()(__x.get(), nullptr);
}
template <class _Tp, class _Dp>
constexpr bool operator<(nullptr_t, const unique_ptr<_Tp, _Dp>& __x) {
  return less<typename unique_ptr<_Tp, _Dp>::pointer>()(nullptr, __x.get());
}
template <class _Tp, class _Dp>
constexpr bool operator>(const unique_ptr<_Tp, _Dp>& __x, nullptr_t) {
  return nullptr < __x;
}
template <class _Tp, class _Dp>
constexpr bool operator>(nullptr_t, const unique_ptr<_Tp, _Dp>& __x) {
  return __x < nullptr;
}
template <class _Tp, class _Dp>
constexpr bool operator<=(const unique_ptr<_Tp, _Dp>& __x, nullptr_t) {
  return !(nullptr < __x);
}
template <class _Tp, class _Dp>
constexpr bool operator<=(nullptr_t, const unique_ptr<_Tp, _Dp>& __x) {
  return !(__x < nullptr);
}
template <class _Tp, class _Dp>
constexpr bool operator>=(const unique_ptr<_Tp, _Dp>& __x, nullptr_t) {
  return !(__x < nullptr);
}
template <class _Tp, class _Dp>
constexpr bool operator>=(nullptr_t, const unique_ptr<_Tp, _Dp>& __x) {
  return !(nullptr < __x);
}
template <class _Tp, class _Dp>
  requires three_way_comparable<typename unique_ptr<_Tp, _Dp>::pointer>
constexpr compare_three_way_result_t<typename unique_ptr<_Tp, _Dp>::pointer> operator<=>(const unique_ptr<_Tp, _Dp>& __x,
                                                                                    nullptr_t) {
  return compare_three_way()(__x.get(), static_cast<typename unique_ptr<_Tp, _Dp>::pointer>(nullptr));
}

// [util.smartptr.hash]: enabled iff hash<pointer> is.
template <class _Tp, class _Dp>
  requires __ycxx::__detail::__hash_enabled<typename unique_ptr<_Tp, _Dp>::pointer>
struct hash<unique_ptr<_Tp, _Dp>> {
  size_t operator()(const unique_ptr<_Tp, _Dp>& p) const
      noexcept(noexcept(hash<typename unique_ptr<_Tp, _Dp>::pointer>()(p.get()))) {
    return hash<typename unique_ptr<_Tp, _Dp>::pointer>()(p.get());
  }
};

// [unique.ptr.io]: written against the declaration of basic_ostream; usable once <ostream> is
// included (anything holding a stream has).
template <class _Ep, class _Tp, class _Yp, class _Dp>
  requires requires(basic_ostream<_Ep, _Tp>& __os, const unique_ptr<_Yp, _Dp>& p) { __os << p.get(); }
basic_ostream<_Ep, _Tp>& operator<<(basic_ostream<_Ep, _Tp>& __os, const unique_ptr<_Yp, _Dp>& p) {
  __os << p.get();
  return __os;
}

}} // namespace std
