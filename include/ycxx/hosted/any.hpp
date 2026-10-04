// libycxx hosted: <any> ([any]). Hosted because a large contained value is heap-allocated.
//
// Layout: a three-pointer buffer plus a pointer to a per-type table of operations. Small-object
// storage is used for types that fit the buffer and are nothrow move constructible
// ([any.class.general]/3); others live on the heap and the buffer holds the pointer.
//
// Type identity: the table's address identifies the contained type. Two copies of one table
// can exist across shared libraries, so with RTTI a mismatch falls back to comparing type_info.
#pragma once

#include <ycxx/core/error.hpp>
#include <ycxx/core/new.hpp>
#include <ycxx/core/typeinfo.hpp>
#include <ycxx/core/utility_base.hpp>
#include <initializer_list>

namespace std {

class bad_any_cast : public bad_cast {
public:
  const char* what() const noexcept override { return "bad any_cast"; }
};

class any;

} // namespace std

namespace ycxx::detail::any_impl {

union storage {
  void* ptr;
  alignas(void*) unsigned char buf[3 * sizeof(void*)];
};

template <class T>
inline constexpr bool stored_inline =
    sizeof(T) <= sizeof(storage) && alignof(storage) % alignof(T) == 0 && std::is_nothrow_move_constructible_v<T>;

struct table {
  void (*destroy)(storage&) noexcept;
  void (*copy)(const storage& src, storage& dst); // constructs dst from src
  void (*move)(storage& src, storage& dst) noexcept; // constructs dst from src, then destroys src
  void* (*get)(storage&) noexcept;
#if YCXX_HAS_RTTI
  const std::type_info& (*type)() noexcept;
#endif
};

template <class T>
struct ops {
  static T* obj(storage& s) noexcept {
    if constexpr (stored_inline<T>)
      return std::launder(reinterpret_cast<T*>(s.buf));
    else
      return static_cast<T*>(s.ptr);
  }
  template <class... Args>
  static void create(storage& s, Args&&... args) {
    if constexpr (stored_inline<T>)
      ::new (static_cast<void*>(s.buf)) T(static_cast<Args&&>(args)...);
    else
      s.ptr = new T(static_cast<Args&&>(args)...);
  }
  static void destroy(storage& s) noexcept {
    if constexpr (stored_inline<T>)
      obj(s)->~T();
    else
      delete obj(s);
  }
  static void copy(const storage& src, storage& dst) { create(dst, *obj(const_cast<storage&>(src))); }
  static void move(storage& src, storage& dst) noexcept {
    if constexpr (stored_inline<T>) {
      ::new (static_cast<void*>(dst.buf)) T(static_cast<T&&>(*obj(src)));
      obj(src)->~T();
    } else {
      dst.ptr = src.ptr;
    }
  }
  static void* get(storage& s) noexcept { return obj(s); }
#if YCXX_HAS_RTTI
  static const std::type_info& type() noexcept { return typeid(T); }
#endif
};

template <class T>
inline constexpr table table_for = {&ops<T>::destroy, &ops<T>::copy, &ops<T>::move, &ops<T>::get,
#if YCXX_HAS_RTTI
                                    &ops<T>::type
#endif
};

template <class T>
inline constexpr bool is_in_place_type_t = false;
template <class T>
inline constexpr bool is_in_place_type_t<std::in_place_type_t<T>> = true;

} // namespace ycxx::detail::any_impl

namespace std {

class any {
  ycxx::detail::any_impl::storage s_;
  const ycxx::detail::any_impl::table* t_ = nullptr;

  template <class T>
  friend const T* any_cast(const any*) noexcept;
  template <class T>
  friend T* any_cast(any*) noexcept;

  template <class VT, class... Args>
  VT& create(Args&&... args) {
    ycxx::detail::any_impl::ops<VT>::create(s_, static_cast<Args&&>(args)...);
    t_ = &ycxx::detail::any_impl::table_for<VT>;
    return *ycxx::detail::any_impl::ops<VT>::obj(s_);
  }
  // Takes over other's value (other is left empty).
  void take(any& other) noexcept {
    if (other.t_) {
      other.t_->move(other.s_, s_);
      t_ = other.t_;
      other.t_ = nullptr;
    }
  }
  template <class T>
  bool holds() const noexcept {
    if (t_ == &ycxx::detail::any_impl::table_for<T>)
      return true;
#if YCXX_HAS_RTTI
    return t_ && t_->type() == typeid(T);
#else
    return false;
#endif
  }

public:
  // ---- [any.cons] ----
  constexpr any() noexcept : s_{} {}
  any(const any& other) {
    if (other.t_) {
      other.t_->copy(other.s_, s_);
      t_ = other.t_;
    }
  }
  any(any&& other) noexcept { take(other); }

  // The constraints are checked by SFINAE on the *class* is_copy_constructible<VT>, not by a
  // requires-clause. Deciding whether a type with a constructor from any (`A(any)`), or
  // tuple<any>, is copyable asks whether any is constructible from that type, which asks the
  // same question again. Through a default template argument the inner query sees an incomplete
  // is_copy_constructible<VT> and quietly drops the candidate. A requires-clause would make the
  // re-entry a hard error ("satisfaction depends on itself") on both compilers.
  template <class T, class VT = decay_t<T>,
            enable_if_t<!is_same_v<VT, any> && !ycxx::detail::any_impl::is_in_place_type_t<VT>, int> = 0,
            enable_if_t<is_copy_constructible<VT>::value, int> = 0>
  any(T&& value) {
    create<VT>(static_cast<T&&>(value));
  }
  template <class T, class... Args, class VT = decay_t<T>>
    requires is_copy_constructible_v<VT> && is_constructible_v<VT, Args...>
  explicit any(in_place_type_t<T>, Args&&... args) {
    create<VT>(static_cast<Args&&>(args)...);
  }
  template <class T, class U, class... Args, class VT = decay_t<T>>
    requires is_copy_constructible_v<VT> && is_constructible_v<VT, initializer_list<U>&, Args...>
  explicit any(in_place_type_t<T>, initializer_list<U> il, Args&&... args) {
    create<VT>(il, static_cast<Args&&>(args)...);
  }

  ~any() { reset(); }

  // ---- [any.assign] ----
  any& operator=(const any& rhs) {
    any(rhs).swap(*this);
    return *this;
  }
  any& operator=(any&& rhs) noexcept {
    any(static_cast<any&&>(rhs)).swap(*this);
    return *this;
  }
  template <class T, class VT = decay_t<T>, enable_if_t<!is_same_v<VT, any>, int> = 0,
            enable_if_t<is_copy_constructible<VT>::value, int> = 0> // see any(T&&)
  any& operator=(T&& rhs) {
    any(static_cast<T&&>(rhs)).swap(*this);
    return *this;
  }

  // ---- [any.modifiers] ----
  template <class T, class... Args, class VT = decay_t<T>>
    requires is_copy_constructible_v<VT> && is_constructible_v<VT, Args...>
  VT& emplace(Args&&... args) {
    reset();
    return create<VT>(static_cast<Args&&>(args)...);
  }
  template <class T, class U, class... Args, class VT = decay_t<T>>
    requires is_copy_constructible_v<VT> && is_constructible_v<VT, initializer_list<U>&, Args...>
  VT& emplace(initializer_list<U> il, Args&&... args) {
    reset();
    return create<VT>(il, static_cast<Args&&>(args)...);
  }
  void reset() noexcept {
    if (t_) {
      t_->destroy(s_);
      t_ = nullptr;
    }
  }
  void swap(any& rhs) noexcept {
    if (this == &rhs)
      return;
    any tmp;
    tmp.take(rhs);
    rhs.take(*this);
    take(tmp);
  }

  // ---- [any.observers] ----
  bool has_value() const noexcept { return t_ != nullptr; }
#if YCXX_HAS_RTTI // typeid cannot even be parsed under -fno-rtti
  const type_info& type() const noexcept { return t_ ? t_->type() : typeid(void); }
#endif
};

// ---- [any.nonmembers] ----
inline void swap(any& x, any& y) noexcept { x.swap(y); }

// Constrained (QoI; the draft only says "Equivalent to"), so make_any is SFINAE-friendly.
template <class T, class... Args>
  requires is_constructible_v<any, in_place_type_t<T>, Args...>
any make_any(Args&&... args) {
  return any(in_place_type<T>, static_cast<Args&&>(args)...);
}
template <class T, class U, class... Args>
  requires is_constructible_v<any, in_place_type_t<T>, initializer_list<U>&, Args...>
any make_any(initializer_list<U> il, Args&&... args) {
  return any(in_place_type<T>, il, static_cast<Args&&>(args)...);
}

template <class T>
const T* any_cast(const any* operand) noexcept {
  static_assert(!is_void_v<T>, "std::any_cast: T must not be void");
  if constexpr (is_object_v<T> && !is_array_v<T>) {
    if (operand && operand->holds<remove_cv_t<T>>())
      return static_cast<const T*>(operand->t_->get(const_cast<ycxx::detail::any_impl::storage&>(operand->s_)));
  }
  return nullptr; // function and array types are never contained
}
template <class T>
T* any_cast(any* operand) noexcept {
  static_assert(!is_void_v<T>, "std::any_cast: T must not be void");
  if constexpr (is_object_v<T> && !is_array_v<T>) {
    if (operand && operand->holds<remove_cv_t<T>>())
      return static_cast<T*>(operand->t_->get(operand->s_));
  }
  return nullptr;
}

} // namespace std

namespace ycxx::detail {
[[noreturn]] [[gnu::cold]] inline void throw_bad_any_cast() {
  raise_with(ycxx_error_bad_any_cast, "std::bad_any_cast", [] { return std::bad_any_cast(); });
}
} // namespace ycxx::detail

namespace std {

template <class T>
T any_cast(const any& operand) {
  using U = remove_cvref_t<T>;
  static_assert(is_constructible_v<T, const U&>, "std::any_cast: T must be constructible from const U&");
  if (const U* p = std::any_cast<U>(&operand))
    return static_cast<T>(*p);
  ycxx::detail::throw_bad_any_cast();
}
template <class T>
T any_cast(any& operand) {
  using U = remove_cvref_t<T>;
  static_assert(is_constructible_v<T, U&>, "std::any_cast: T must be constructible from U&");
  if (U* p = std::any_cast<U>(&operand))
    return static_cast<T>(*p);
  ycxx::detail::throw_bad_any_cast();
}
template <class T>
T any_cast(any&& operand) {
  using U = remove_cvref_t<T>;
  static_assert(is_constructible_v<T, U>, "std::any_cast: T must be constructible from U");
  if (U* p = std::any_cast<U>(&operand))
    return static_cast<T>(static_cast<U&&>(*p));
  ycxx::detail::throw_bad_any_cast();
}

} // namespace std
