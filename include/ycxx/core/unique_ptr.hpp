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

namespace std {

// [unique.ptr.dltr.dflt]
template <class T>
struct default_delete {
  constexpr default_delete() noexcept = default;
  template <class U>
    requires is_convertible_v<U*, T*>
  constexpr default_delete(const default_delete<U>&) noexcept {}

  constexpr void operator()(T* ptr) const {
    static_assert(!is_void_v<T>, "std::default_delete: cannot delete a pointer to void");
    static_assert(requires { sizeof(T); }, "std::default_delete: T must be a complete type");
    delete ptr;
  }
};

// [unique.ptr.dltr.dflt1]
template <class T>
struct default_delete<T[]> {
  constexpr default_delete() noexcept = default;
  template <class U>
    requires is_convertible_v<U (*)[], T (*)[]>
  constexpr default_delete(const default_delete<U[]>&) noexcept {}

  template <class U>
    requires is_convertible_v<U (*)[], T (*)[]>
  constexpr void operator()(U* ptr) const {
    static_assert(requires { sizeof(U); }, "std::default_delete<T[]>: U must be a complete type");
    delete[] ptr;
  }
};

} // namespace std

namespace ycxx::detail {

// unique_ptr<T, D>::pointer ([unique.ptr.single.general]/4).
template <class T, class D>
struct uptr_pointer {
  using type = T*;
};
template <class T, class D>
  requires requires { typename std::remove_reference_t<D>::pointer; }
struct uptr_pointer<T, D> {
  using type = typename std::remove_reference_t<D>::pointer;
};

// "either D is a reference type and E is the same type as D, or D is not a reference type and
// E is implicitly convertible to D" ([unique.ptr.single.ctor]/19.3).
template <class D, class E>
concept uptr_deleter_from = (std::is_reference_v<D> ? std::is_same_v<E, D> : std::is_convertible_v<E, D>);

// V(*)[] is convertible to element_type(*)[] for a pointer type U = V*; the array forms'
// constructor and reset constraints ([unique.ptr.runtime.ctor]/2).
template <class U, class Pointer, class Elem>
concept uptr_array_ptr = std::is_same_v<U, Pointer> ||
                         (std::is_same_v<Pointer, Elem*> && std::is_pointer_v<U> &&
                          std::is_convertible_v<std::remove_pointer_t<U> (*)[], Elem (*)[]>);

} // namespace ycxx::detail

namespace std {

// [unique.ptr.single]
template <class T, class D = default_delete<T>>
class unique_ptr {
public:
  using pointer = typename ycxx::detail::uptr_pointer<T, D>::type;
  using element_type = T;
  using deleter_type = D;

private:
  template <class, class>
  friend class unique_ptr;

  pointer p_;
  [[no_unique_address]] D d_;

public:
  // [unique.ptr.single.ctor]
  constexpr unique_ptr() noexcept
    requires(!is_pointer_v<D> && is_default_constructible_v<D>)
      : p_(), d_() {}
  constexpr unique_ptr(nullptr_t) noexcept
    requires(!is_pointer_v<D> && is_default_constructible_v<D>)
      : p_(), d_() {}
  constexpr explicit unique_ptr(type_identity_t<pointer> p) noexcept
    requires(!is_pointer_v<D> && is_default_constructible_v<D>)
      : p_(p), d_() {}
  constexpr unique_ptr(type_identity_t<pointer> p, const D& d) noexcept
    requires is_constructible_v<D, const D&>
      : p_(p), d_(d) {}
  constexpr unique_ptr(type_identity_t<pointer> p, remove_reference_t<D>&& d) noexcept
    requires(!is_reference_v<D> && is_constructible_v<D, D &&>)
      : p_(p), d_(static_cast<D&&>(d)) {}
  unique_ptr(type_identity_t<pointer>, remove_reference_t<D>&&)
    requires is_reference_v<D>
  = delete;
  constexpr unique_ptr(unique_ptr&& u) noexcept
    requires is_move_constructible_v<D>
      : p_(u.release()), d_(static_cast<D&&>(u.d_)) {}
  template <class U, class E>
    requires(is_convertible_v<typename unique_ptr<U, E>::pointer, pointer> && !is_array_v<U> &&
             ycxx::detail::uptr_deleter_from<D, E>)
  constexpr unique_ptr(unique_ptr<U, E>&& u) noexcept : p_(u.release()), d_(static_cast<E&&>(u.d_)) {}

  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  // [unique.ptr.single.dtor]
  constexpr ~unique_ptr() {
    if (p_ != nullptr)
      d_(p_);
  }

  // [unique.ptr.single.asgn]
  constexpr unique_ptr& operator=(unique_ptr&& u) noexcept
    requires is_move_assignable_v<D>
  {
    reset(u.release());
    d_ = static_cast<D&&>(u.d_);
    return *this;
  }
  template <class U, class E>
    requires(is_convertible_v<typename unique_ptr<U, E>::pointer, pointer> && !is_array_v<U> &&
             is_assignable_v<D&, E &&>)
  constexpr unique_ptr& operator=(unique_ptr<U, E>&& u) noexcept {
    reset(u.release());
    d_ = static_cast<E&&>(u.d_);
    return *this;
  }
  constexpr unique_ptr& operator=(nullptr_t) noexcept {
    reset();
    return *this;
  }

  // [unique.ptr.single.observers]
  constexpr add_lvalue_reference_t<T> operator*() const noexcept(noexcept(*declval<pointer>()))
    requires requires { *declval<pointer>(); }
  {
    static_assert(!reference_converts_from_temporary_v<add_lvalue_reference_t<T>, decltype(*declval<pointer>())>,
                  "std::unique_ptr::operator*: would bind a reference to a temporary");
    ycxx::detail::precondition(p_ != nullptr, "std::unique_ptr::operator*: null pointer");
    return *p_;
  }
  constexpr pointer operator->() const noexcept {
    ycxx::detail::precondition(p_ != nullptr, "std::unique_ptr::operator->: null pointer");
    return p_;
  }
  constexpr pointer get() const noexcept { return p_; }
  constexpr deleter_type& get_deleter() noexcept { return d_; }
  constexpr const deleter_type& get_deleter() const noexcept { return d_; }
  constexpr explicit operator bool() const noexcept { return p_ != nullptr; }

  // [unique.ptr.single.modifiers]
  constexpr pointer release() noexcept {
    pointer old = p_;
    p_ = pointer();
    return old;
  }
  constexpr void reset(pointer p = pointer()) noexcept {
    pointer old = p_;
    p_ = p;
    if (old != nullptr)
      d_(old);
  }
  constexpr void swap(unique_ptr& u) noexcept {
    ycxx::detail::swap_adl::do_swap(p_, u.p_);
    ycxx::detail::swap_adl::do_swap(d_, u.d_);
  }
};

// [unique.ptr.runtime]
template <class T, class D>
class unique_ptr<T[], D> {
public:
  using pointer = typename ycxx::detail::uptr_pointer<T, D>::type;
  using element_type = T;
  using deleter_type = D;

private:
  template <class, class>
  friend class unique_ptr;

  // Constraints of the converting constructor and assignment ([unique.ptr.runtime.ctor]/6).
  template <class U, class E>
  static constexpr bool convertible_from =
      is_array_v<U> && is_same_v<pointer, T*> &&
      is_same_v<typename unique_ptr<U, E>::pointer, typename unique_ptr<U, E>::element_type*> &&
      is_convertible_v<typename unique_ptr<U, E>::element_type (*)[], T (*)[]>;

  pointer p_;
  [[no_unique_address]] D d_;

public:
  // [unique.ptr.runtime.ctor]
  constexpr unique_ptr() noexcept
    requires(!is_pointer_v<D> && is_default_constructible_v<D>)
      : p_(), d_() {}
  constexpr unique_ptr(nullptr_t) noexcept
    requires(!is_pointer_v<D> && is_default_constructible_v<D>)
      : p_(), d_() {}
  template <class U>
    requires(!is_pointer_v<D> && is_default_constructible_v<D> && ycxx::detail::uptr_array_ptr<U, pointer, T>)
  constexpr explicit unique_ptr(U p) noexcept : p_(p), d_() {}
  template <class U>
    requires((ycxx::detail::uptr_array_ptr<U, pointer, T> || is_same_v<U, nullptr_t>) &&
             is_constructible_v<D, const D&>)
  constexpr unique_ptr(U p, const D& d) noexcept : p_(p), d_(d) {}
  template <class U>
    requires((ycxx::detail::uptr_array_ptr<U, pointer, T> || is_same_v<U, nullptr_t>) && !is_reference_v<D> &&
             is_constructible_v<D, D &&>)
  constexpr unique_ptr(U p, remove_reference_t<D>&& d) noexcept : p_(p), d_(static_cast<D&&>(d)) {}
  template <class U>
    requires((ycxx::detail::uptr_array_ptr<U, pointer, T> || is_same_v<U, nullptr_t>) && is_reference_v<D>)
  unique_ptr(U, remove_reference_t<D>&&) = delete;
  constexpr unique_ptr(unique_ptr&& u) noexcept
    requires is_move_constructible_v<D>
      : p_(u.release()), d_(static_cast<D&&>(u.d_)) {}
  template <class U, class E>
    requires(convertible_from<U, E> && ycxx::detail::uptr_deleter_from<D, E>)
  constexpr unique_ptr(unique_ptr<U, E>&& u) noexcept : p_(u.release()), d_(static_cast<E&&>(u.d_)) {}

  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  constexpr ~unique_ptr() {
    if (p_ != nullptr)
      d_(p_);
  }

  // [unique.ptr.runtime.asgn]
  constexpr unique_ptr& operator=(unique_ptr&& u) noexcept
    requires is_move_assignable_v<D>
  {
    reset(u.release());
    d_ = static_cast<D&&>(u.d_);
    return *this;
  }
  template <class U, class E>
    requires(convertible_from<U, E> && is_assignable_v<D&, E &&>)
  constexpr unique_ptr& operator=(unique_ptr<U, E>&& u) noexcept {
    reset(u.release());
    d_ = static_cast<E&&>(u.d_);
    return *this;
  }
  constexpr unique_ptr& operator=(nullptr_t) noexcept {
    reset();
    return *this;
  }

  // [unique.ptr.runtime.observers]
  constexpr T& operator[](size_t i) const {
    ycxx::detail::precondition(p_ != nullptr, "std::unique_ptr<T[]>::operator[]: null pointer");
    return p_[i];
  }
  constexpr pointer get() const noexcept { return p_; }
  constexpr deleter_type& get_deleter() noexcept { return d_; }
  constexpr const deleter_type& get_deleter() const noexcept { return d_; }
  constexpr explicit operator bool() const noexcept { return p_ != nullptr; }

  // [unique.ptr.runtime.modifiers]
  constexpr pointer release() noexcept {
    pointer old = p_;
    p_ = pointer();
    return old;
  }
  template <class U>
    requires ycxx::detail::uptr_array_ptr<U, pointer, T>
  constexpr void reset(U p) noexcept {
    pointer old = p_;
    p_ = p;
    if (old != nullptr)
      d_(old);
  }
  constexpr void reset(nullptr_t = nullptr) noexcept { reset(pointer()); }
  constexpr void swap(unique_ptr& u) noexcept {
    ycxx::detail::swap_adl::do_swap(p_, u.p_);
    ycxx::detail::swap_adl::do_swap(d_, u.d_);
  }
};

// [unique.ptr.create]
template <class T, class... Args>
  requires(!is_array_v<T>)
constexpr unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>(new T(static_cast<Args&&>(args)...));
}
template <class T>
  requires is_unbounded_array_v<T>
constexpr unique_ptr<T> make_unique(size_t n) {
  return unique_ptr<T>(new remove_extent_t<T>[n]());
}
template <class T, class... Args>
  requires is_bounded_array_v<T>
void make_unique(Args&&...) = delete;

template <class T>
  requires(!is_array_v<T>)
constexpr unique_ptr<T> make_unique_for_overwrite() {
  return unique_ptr<T>(new T);
}
template <class T>
  requires is_unbounded_array_v<T>
constexpr unique_ptr<T> make_unique_for_overwrite(size_t n) {
  return unique_ptr<T>(new remove_extent_t<T>[n]);
}
template <class T, class... Args>
  requires is_bounded_array_v<T>
void make_unique_for_overwrite(Args&&...) = delete;

// [unique.ptr.special]
template <class T, class D>
  requires is_swappable_v<D>
constexpr void swap(unique_ptr<T, D>& x, unique_ptr<T, D>& y) noexcept {
  x.swap(y);
}

template <class T1, class D1, class T2, class D2>
constexpr bool operator==(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y) {
  return x.get() == y.get();
}
template <class T1, class D1, class T2, class D2>
constexpr bool operator<(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y) {
  using CT = common_type_t<typename unique_ptr<T1, D1>::pointer, typename unique_ptr<T2, D2>::pointer>;
  static_assert(is_convertible_v<typename unique_ptr<T1, D1>::pointer, CT> &&
                    is_convertible_v<typename unique_ptr<T2, D2>::pointer, CT>,
                "std::unique_ptr operator<: pointers must convert to their common type");
  return less<CT>()(x.get(), y.get());
}
template <class T1, class D1, class T2, class D2>
constexpr bool operator>(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y) {
  return y < x;
}
template <class T1, class D1, class T2, class D2>
constexpr bool operator<=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y) {
  return !(y < x);
}
template <class T1, class D1, class T2, class D2>
constexpr bool operator>=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y) {
  return !(x < y);
}
template <class T1, class D1, class T2, class D2>
  requires three_way_comparable_with<typename unique_ptr<T1, D1>::pointer, typename unique_ptr<T2, D2>::pointer>
constexpr compare_three_way_result_t<typename unique_ptr<T1, D1>::pointer, typename unique_ptr<T2, D2>::pointer>
operator<=>(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y) {
  return compare_three_way()(x.get(), y.get());
}

template <class T, class D>
constexpr bool operator==(const unique_ptr<T, D>& x, nullptr_t) noexcept {
  return !x;
}
template <class T, class D>
constexpr bool operator<(const unique_ptr<T, D>& x, nullptr_t) {
  return less<typename unique_ptr<T, D>::pointer>()(x.get(), nullptr);
}
template <class T, class D>
constexpr bool operator<(nullptr_t, const unique_ptr<T, D>& x) {
  return less<typename unique_ptr<T, D>::pointer>()(nullptr, x.get());
}
template <class T, class D>
constexpr bool operator>(const unique_ptr<T, D>& x, nullptr_t) {
  return nullptr < x;
}
template <class T, class D>
constexpr bool operator>(nullptr_t, const unique_ptr<T, D>& x) {
  return x < nullptr;
}
template <class T, class D>
constexpr bool operator<=(const unique_ptr<T, D>& x, nullptr_t) {
  return !(nullptr < x);
}
template <class T, class D>
constexpr bool operator<=(nullptr_t, const unique_ptr<T, D>& x) {
  return !(x < nullptr);
}
template <class T, class D>
constexpr bool operator>=(const unique_ptr<T, D>& x, nullptr_t) {
  return !(x < nullptr);
}
template <class T, class D>
constexpr bool operator>=(nullptr_t, const unique_ptr<T, D>& x) {
  return !(nullptr < x);
}
template <class T, class D>
  requires three_way_comparable<typename unique_ptr<T, D>::pointer>
constexpr compare_three_way_result_t<typename unique_ptr<T, D>::pointer> operator<=>(const unique_ptr<T, D>& x,
                                                                                    nullptr_t) {
  return compare_three_way()(x.get(), static_cast<typename unique_ptr<T, D>::pointer>(nullptr));
}

// [util.smartptr.hash]: enabled iff hash<pointer> is.
template <class T, class D>
  requires ycxx::detail::hash_enabled<typename unique_ptr<T, D>::pointer>
struct hash<unique_ptr<T, D>> {
  size_t operator()(const unique_ptr<T, D>& p) const
      noexcept(noexcept(hash<typename unique_ptr<T, D>::pointer>()(p.get()))) {
    return hash<typename unique_ptr<T, D>::pointer>()(p.get());
  }
};

// [unique.ptr.io]: written against the declaration of basic_ostream; usable once <ostream> is
// included (anything holding a stream has).
template <class E, class T, class Y, class D>
  requires requires(basic_ostream<E, T>& os, const unique_ptr<Y, D>& p) { os << p.get(); }
basic_ostream<E, T>& operator<<(basic_ostream<E, T>& os, const unique_ptr<Y, D>& p) {
  os << p.get();
  return os;
}

} // namespace std
