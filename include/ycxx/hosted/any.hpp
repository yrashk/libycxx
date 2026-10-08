// libycxx hosted: <any> ([any]). Hosted because a large contained value is heap-allocated.
//
// Layout: a three-pointer buffer plus a pointer to a per-type table of operations. Small-object
// storage is used for types that fit the buffer and are nothrow move constructible
// ([any.class.general]/3); others live on the heap and the buffer holds the pointer.
//
// Type identity: the table's address identifies the contained type. Two copies of one table
// can exist across shared libraries, so with RTTI a mismatch falls back to comparing type_info.
// Without RTTI, any_cast across a shared library built with hidden visibility or -Bsymbolic
// does not recognise the type (STATUS: known limitations).
#pragma once

#include <ycxx/core/error.hpp>
#include <ycxx/core/new.hpp>
#include <ycxx/core/typeinfo.hpp>
#include <ycxx/core/utility_base.hpp>
#include <initializer_list>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

class bad_any_cast : public bad_cast {
public:
  const char* what() const noexcept override { return "bad any_cast"; }
};

class any;

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__any_impl {

union __storage {
  void* ptr;
  alignas(void*) unsigned char __buf[3 * sizeof(void*)];
};

template <class _Tp>
inline constexpr bool __stored_inline =
    sizeof(_Tp) <= sizeof(__storage) && alignof(__storage) % alignof(_Tp) == 0 && std::is_nothrow_move_constructible_v<_Tp>;

struct table {
  void (*destroy)(__storage&) noexcept;
  void (*copy)(const __storage& __src, __storage& __dst); // constructs dst from src
  void (*move)(__storage& __src, __storage& __dst) noexcept; // constructs dst from src, then destroys src
  void* (*get)(__storage&) noexcept;
#if _YCXX_HAS_RTTI
  const std::type_info& (*type)() noexcept;
#endif
};

template <class _Tp>
struct __ops {
  static _Tp* __obj(__storage& s) noexcept {
    if constexpr (__stored_inline<_Tp>)
      return std::launder(reinterpret_cast<_Tp*>(s.__buf));
    else
      return static_cast<_Tp*>(s.ptr);
  }
  template <class... _Args>
  static void __create(__storage& s, _Args&&... __args) {
    if constexpr (__stored_inline<_Tp>)
      ::new (static_cast<void*>(s.__buf)) _Tp(static_cast<_Args&&>(__args)...);
    else
      // A plain new-expression: a class-specific operator new/delete is honoured (a type that
      // deletes its operator new is not meant to live on the heap, so it cannot be stored).
      s.ptr = new _Tp(static_cast<_Args&&>(__args)...);
  }
  static void destroy(__storage& s) noexcept {
    if constexpr (__stored_inline<_Tp>)
      __obj(s)->~_Tp();
    else
      delete __obj(s);
  }
  // [any.cons]/2: copies from any_cast<const T&>(other), so the const T& constructor is chosen.
  static void copy(const __storage& __src, __storage& __dst) {
    __create(__dst, *static_cast<const _Tp*>(__obj(const_cast<__storage&>(__src))));
  }
  static void move(__storage& __src, __storage& __dst) noexcept {
    if constexpr (__stored_inline<_Tp>) {
      ::new (static_cast<void*>(__dst.__buf)) _Tp(static_cast<_Tp&&>(*__obj(__src)));
      __obj(__src)->~_Tp();
    } else {
      __dst.ptr = __src.ptr;
    }
  }
  static void* get(__storage& s) noexcept { return __obj(s); }
#if _YCXX_HAS_RTTI
  static const std::type_info& type() noexcept { return typeid(_Tp); }
#endif
};

template <class _Tp>
inline constexpr table __table_for = {&__ops<_Tp>::destroy, &__ops<_Tp>::copy, &__ops<_Tp>::move, &__ops<_Tp>::get,
#if _YCXX_HAS_RTTI
                                    &__ops<_Tp>::type
#endif
};

template <class _Tp>
inline constexpr bool __is_in_place_type_t = false;
template <class _Tp>
inline constexpr bool __is_in_place_type_t<std::in_place_type_t<_Tp>> = true;

}} // namespace __ycxx::__detail::__any_impl

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

class any {
  __ycxx::__detail::__any_impl::__storage __s_;
  const __ycxx::__detail::__any_impl::table* __t_ = nullptr;

  template <class _Tp>
  friend const _Tp* any_cast(const any*) noexcept;
  template <class _Tp>
  friend _Tp* any_cast(any*) noexcept;

  template <class _VT, class... _Args>
  _VT& __create(_Args&&... __args) {
    __ycxx::__detail::__any_impl::__ops<_VT>::__create(__s_, static_cast<_Args&&>(__args)...);
    __t_ = &__ycxx::__detail::__any_impl::__table_for<_VT>;
    return *__ycxx::__detail::__any_impl::__ops<_VT>::__obj(__s_);
  }
  // Takes over other's value (other is left empty).
  void take(any& other) noexcept {
    if (other.__t_) {
      other.__t_->move(other.__s_, __s_);
      __t_ = other.__t_;
      other.__t_ = nullptr;
    }
  }
  template <class _Tp>
  bool __holds() const noexcept {
    // Only copy-constructible types are ever stored ([any.class.general]/4); for any other T,
    // naming table_for<T> would instantiate its copy operation ([any.nonmembers]/9-10 only
    // Mandate !is_void_v<T>).
    if constexpr (!is_copy_constructible_v<_Tp>) {
      return false;
    } else {
      if (__t_ == &__ycxx::__detail::__any_impl::__table_for<_Tp>)
        return true;
#if _YCXX_HAS_RTTI
      return __t_ && __t_->type() == typeid(_Tp);
#else
      return false;
#endif
    }
  }

public:
  // ---- [any.cons] ----
  constexpr any() noexcept : __s_{} {}
  any(const any& other) {
    if (other.__t_) {
      other.__t_->copy(other.__s_, __s_);
      __t_ = other.__t_;
    }
  }
  any(any&& other) noexcept { take(other); }

  // The constraints are checked by SFINAE on the *class* is_copy_constructible<VT>, not by a
  // requires-clause. Deciding whether a type with a constructor from any (`_Ap(any)`), or
  // tuple<any>, is copyable asks whether any is constructible from that type, which asks the
  // same question again. Through a default template argument the inner query sees an incomplete
  // is_copy_constructible<VT> and quietly drops the candidate. A requires-clause would make the
  // re-entry a hard error ("satisfaction depends on itself") on both compilers.
  template <class _Tp, class _VT = decay_t<_Tp>,
            enable_if_t<!is_same_v<_VT, any> && !__ycxx::__detail::__any_impl::__is_in_place_type_t<_VT>, int> = 0,
            enable_if_t<is_copy_constructible<_VT>::value, int> = 0>
  any(_Tp&& value) {
    __create<_VT>(static_cast<_Tp&&>(value));
  }
  template <class _Tp, class... _Args, class _VT = decay_t<_Tp>>
    requires is_copy_constructible_v<_VT> && is_constructible_v<_VT, _Args...>
  explicit any(in_place_type_t<_Tp>, _Args&&... __args) {
    __create<_VT>(static_cast<_Args&&>(__args)...);
  }
  template <class _Tp, class _Up, class... _Args, class _VT = decay_t<_Tp>>
    requires is_copy_constructible_v<_VT> && is_constructible_v<_VT, initializer_list<_Up>&, _Args...>
  explicit any(in_place_type_t<_Tp>, initializer_list<_Up> il, _Args&&... __args) {
    __create<_VT>(il, static_cast<_Args&&>(__args)...);
  }

  ~any() { reset(); }

  // ---- [any.assign] ----
  any& operator=(const any& __rhs) {
    any(__rhs).swap(*this);
    return *this;
  }
  any& operator=(any&& __rhs) noexcept {
    any(static_cast<any&&>(__rhs)).swap(*this);
    return *this;
  }
  template <class _Tp, class _VT = decay_t<_Tp>, enable_if_t<!is_same_v<_VT, any>, int> = 0,
            enable_if_t<is_copy_constructible<_VT>::value, int> = 0> // see any(T&&)
  any& operator=(_Tp&& __rhs) {
    // Not any(rhs): for VT = in_place_type_t<X> that would pick the in_place constructor and
    // store an X instead of the tag ([any.assign]/10).
    any __tmp;
    __tmp.__create<_VT>(static_cast<_Tp&&>(__rhs));
    __tmp.swap(*this);
    return *this;
  }

  // ---- [any.modifiers] ----
  template <class _Tp, class... _Args, class _VT = decay_t<_Tp>>
    requires is_copy_constructible_v<_VT> && is_constructible_v<_VT, _Args...>
  _VT& emplace(_Args&&... __args) {
    reset();
    return __create<_VT>(static_cast<_Args&&>(__args)...);
  }
  template <class _Tp, class _Up, class... _Args, class _VT = decay_t<_Tp>>
    requires is_copy_constructible_v<_VT> && is_constructible_v<_VT, initializer_list<_Up>&, _Args...>
  _VT& emplace(initializer_list<_Up> il, _Args&&... __args) {
    reset();
    return __create<_VT>(il, static_cast<_Args&&>(__args)...);
  }
  void reset() noexcept {
    if (__t_) {
      __t_->destroy(__s_);
      __t_ = nullptr;
    }
  }
  void swap(any& __rhs) noexcept {
    if (this == &__rhs)
      return;
    any __tmp;
    __tmp.take(__rhs);
    __rhs.take(*this);
    take(__tmp);
  }

  // ---- [any.observers] ----
  bool has_value() const noexcept { return __t_ != nullptr; }
#if _YCXX_HAS_RTTI // typeid cannot even be parsed under -fno-rtti
  const type_info& type() const noexcept { return __t_ ? __t_->type() : typeid(void); }
#endif
};

// ---- [any.nonmembers] ----
inline void swap(any& __x, any& y) noexcept { __x.swap(y); }

// Constrained (QoI; the draft only says "Equivalent to"), so make_any is SFINAE-friendly.
template <class _Tp, class... _Args>
  requires is_constructible_v<any, in_place_type_t<_Tp>, _Args...>
any make_any(_Args&&... __args) {
  return any(in_place_type<_Tp>, static_cast<_Args&&>(__args)...);
}
template <class _Tp, class _Up, class... _Args>
  requires is_constructible_v<any, in_place_type_t<_Tp>, initializer_list<_Up>&, _Args...>
any make_any(initializer_list<_Up> il, _Args&&... __args) {
  return any(in_place_type<_Tp>, il, static_cast<_Args&&>(__args)...);
}

template <class _Tp>
const _Tp* any_cast(const any* __operand) noexcept {
  static_assert(!is_void_v<_Tp>, "std::any_cast: T must not be void");
  if constexpr (is_object_v<_Tp> && !is_array_v<_Tp>) {
    if (__operand && __operand->__holds<remove_cv_t<_Tp>>())
      return static_cast<const _Tp*>(__operand->__t_->get(const_cast<__ycxx::__detail::__any_impl::__storage&>(__operand->__s_)));
  }
  return nullptr; // function and array types are never contained
}
template <class _Tp>
_Tp* any_cast(any* __operand) noexcept {
  static_assert(!is_void_v<_Tp>, "std::any_cast: T must not be void");
  if constexpr (is_object_v<_Tp> && !is_array_v<_Tp>) {
    if (__operand && __operand->__holds<remove_cv_t<_Tp>>())
      return static_cast<_Tp*>(__operand->__t_->get(__operand->__s_));
  }
  return nullptr;
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
[[noreturn]] [[__gnu__::__cold__]] inline void __throw_bad_any_cast() {
  ::__ycxx::__detail::__raise_with(ycxx_error_bad_any_cast, "std::bad_any_cast", [] { return std::bad_any_cast(); });
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp>
_Tp any_cast(const any& __operand) {
  using _Up = remove_cvref_t<_Tp>;
  static_assert(is_constructible_v<_Tp, const _Up&>, "std::any_cast: T must be constructible from const U&");
  if (const _Up* p = std::any_cast<_Up>(&__operand))
    return static_cast<_Tp>(*p);
  __ycxx::__detail::__throw_bad_any_cast();
}
template <class _Tp>
_Tp any_cast(any& __operand) {
  using _Up = remove_cvref_t<_Tp>;
  static_assert(is_constructible_v<_Tp, _Up&>, "std::any_cast: T must be constructible from U&");
  if (_Up* p = std::any_cast<_Up>(&__operand))
    return static_cast<_Tp>(*p);
  __ycxx::__detail::__throw_bad_any_cast();
}
template <class _Tp>
_Tp any_cast(any&& __operand) {
  using _Up = remove_cvref_t<_Tp>;
  static_assert(is_constructible_v<_Tp, _Up>, "std::any_cast: T must be constructible from U");
  if (_Up* p = std::any_cast<_Up>(&__operand))
    return static_cast<_Tp>(static_cast<_Up&&>(*p));
  __ycxx::__detail::__throw_bad_any_cast();
}

}} // namespace std
