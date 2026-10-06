// libycxx core: atomic, atomic_ref, atomic_flag, the non-member functions and fences
// ([atomics]). The operations are those of ycxx/core/atomic_base.hpp (lock-free through the
// compiler's __atomic builtins, lock-based otherwise; waits through the runtime's slot table).
//
// The volatile overloads participate in overload resolution also for types that are not always
// lock-free ([depr.atomics.volatile]). atomic<T> for integral, floating-point and pointer T are
// partial specializations constrained on T (observably the same as the draft's explicit
// specializations).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/atomic_base.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/type_traits.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Vp>
concept __atomic_integral = is_integral_v<_Vp> && !std::is_same_v<_Vp, bool>;
template <class _Vp>
concept __atomic_floating = __is_floating_v<_Vp>;

// [conv.qual]/2: similar types are the same after removing the cv-qualifiers of every level of
// pointers, pointers to members and arrays, where "array of N" and "array of unknown bound" match.
template <class _Tp>
struct __qual_stripped {
  using type = _Tp;
};
template <class _Tp>
struct __qual_stripped<_Tp*> {
  using type = typename __qual_stripped<std::remove_cv_t<_Tp>>::type*;
};
template <class _Tp, class _Cp>
struct __qual_stripped<_Tp _Cp::*> {
  using type = typename __qual_stripped<std::remove_cv_t<_Tp>>::type _Cp::*;
};
template <class _Tp, std::size_t _Np>
struct __qual_stripped<_Tp[_Np]> {
  using type = typename __qual_stripped<std::remove_cv_t<_Tp>>::type[];
};
template <class _Tp>
struct __qual_stripped<_Tp[]> {
  using type = typename __qual_stripped<std::remove_cv_t<_Tp>>::type[];
};
template <class _Tp, class _Up>
concept __similar_types = std::is_same_v<typename __qual_stripped<std::remove_cv_t<_Tp>>::type,
                                       typename __qual_stripped<std::remove_cv_t<_Up>>::type>;

// x + n / x - n modulo 2^N, for the return values of ++, --, +=, -= ([atomics.types.int]/8).
template <class _Vp>
constexpr _Vp __atomic_wrap_add(_Vp __x, _Vp n) noexcept {
  using _Up = std::make_unsigned_t<_Vp>;
  return static_cast<_Vp>(static_cast<_Up>(static_cast<_Up>(__x) + static_cast<_Up>(n)));
}
template <class _Vp>
constexpr _Vp __atomic_wrap_sub(_Vp __x, _Vp n) noexcept {
  using _Up = std::make_unsigned_t<_Vp>;
  return static_cast<_Vp>(static_cast<_Up>(static_cast<_Up>(__x) - static_cast<_Up>(n)));
}
// p + n elements; at run time computed on the integer value, so that an out-of-range result is
// the pointer of that address ([atomics.types.pointer]/2) rather than undefined.
template <class _Vp>
constexpr _Vp __atomic_ptr_add(_Vp p, std::ptrdiff_t n) noexcept {
  if consteval {
    return p + n;
  } else {
    using _Ep = std::remove_pointer_t<_Vp>;
    return reinterpret_cast<_Vp>(reinterpret_cast<__UINTPTR_TYPE__>(p) +
                               static_cast<__UINTPTR_TYPE__>(n) * static_cast<__UINTPTR_TYPE__>(sizeof(_Ep)));
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// The storage and the operations every atomic<T> has.
template <class _Tp>
struct __atomic_base {
  static_assert(sizeof(_Tp) != 0, "atomic<T>: zero-sized types (a GNU extension) are not supported");
  alignas(::__ycxx::__detail::__atomic_object_align<_Tp>) _Tp __v_;

  constexpr __atomic_base() noexcept(std::is_nothrow_default_constructible_v<_Tp>) : __v_() { clear(); }
  constexpr __atomic_base(_Tp __desired) noexcept : __v_(__desired) { clear(); }
  __atomic_base(const __atomic_base&) = delete;
  __atomic_base& operator=(const __atomic_base&) = delete;
  __atomic_base& operator=(const __atomic_base&) volatile = delete;

  // Stored values have zero padding bits, so compare-and-exchange rarely needs its retry.
  constexpr void clear() noexcept {
    if !consteval {
      if constexpr (::__ycxx::__detail::__atomic_padded<_Tp>)
        __builtin_clear_padding(__builtin_addressof(__v_));
    }
  }

  static constexpr bool is_always_lock_free = ::__ycxx::__detail::__atomic_lock_free<_Tp>;
  bool is_lock_free() const volatile noexcept { return is_always_lock_free; }
  bool is_lock_free() const noexcept { return is_always_lock_free; }

  _Tp load(std::memory_order __o = std::memory_order::seq_cst) const volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::atomic_load<_Tp>(__builtin_addressof(__v_), __o);
  }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp load(std::memory_order __o = std::memory_order::seq_cst) const volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::atomic_load<_Tp>(__builtin_addressof(__v_), __o);
  }
  constexpr _Tp load(std::memory_order __o = std::memory_order::seq_cst) const noexcept {
    return ::__ycxx::__detail::atomic_load<_Tp>(__builtin_addressof(__v_), __o);
  }
  operator _Tp() const volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return load(); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  operator _Tp() const volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return load(); }
  constexpr operator _Tp() const noexcept { return load(); }
  void store(_Tp __desired, std::memory_order __o = std::memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::atomic_store<_Tp>(__builtin_addressof(__v_), __desired, __o);
  }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  void store(_Tp __desired, std::memory_order __o = std::memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::atomic_store<_Tp>(__builtin_addressof(__v_), __desired, __o);
  }
  constexpr void store(_Tp __desired, std::memory_order __o = std::memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::atomic_store<_Tp>(__builtin_addressof(__v_), __desired, __o);
  }
  _Tp operator=(_Tp __desired) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    store(__desired);
    return __desired;
  }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator=(_Tp __desired) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    store(__desired);
    return __desired;
  }
  constexpr _Tp operator=(_Tp __desired) noexcept {
    store(__desired);
    return __desired;
  }
  _Tp exchange(_Tp __desired, std::memory_order __o = std::memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::atomic_exchange<_Tp>(__builtin_addressof(__v_), __desired, __o);
  }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp exchange(_Tp __desired, std::memory_order __o = std::memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::atomic_exchange<_Tp>(__builtin_addressof(__v_), __desired, __o);
  }
  constexpr _Tp exchange(_Tp __desired, std::memory_order __o = std::memory_order::seq_cst) noexcept {
    return ::__ycxx::__detail::atomic_exchange<_Tp>(__builtin_addressof(__v_), __desired, __o);
  }
  bool compare_exchange_weak(_Tp& expected, _Tp __desired, std::memory_order s, std::memory_order __f) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, true, s, __f);
  }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  bool compare_exchange_weak(_Tp& expected, _Tp __desired, std::memory_order s, std::memory_order __f) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, true, s, __f);
  }
  constexpr bool compare_exchange_weak(_Tp& expected, _Tp __desired, std::memory_order s, std::memory_order __f) noexcept {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, true, s, __f);
  }
  bool compare_exchange_strong(_Tp& expected, _Tp __desired, std::memory_order s, std::memory_order __f) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, false, s, __f);
  }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  bool compare_exchange_strong(_Tp& expected, _Tp __desired, std::memory_order s, std::memory_order __f) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, false, s, __f);
  }
  constexpr bool compare_exchange_strong(_Tp& expected, _Tp __desired, std::memory_order s, std::memory_order __f) noexcept {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, false, s, __f);
  }
  bool compare_exchange_weak(_Tp& expected, _Tp __desired, std::memory_order __o = std::memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, true, __o,
                                                      ::__ycxx::__detail::__atomic_failure_order(__o));
  }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  bool compare_exchange_weak(_Tp& expected, _Tp __desired, std::memory_order __o = std::memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, true, __o,
                                                      ::__ycxx::__detail::__atomic_failure_order(__o));
  }
  constexpr bool compare_exchange_weak(_Tp& expected, _Tp __desired, std::memory_order __o = std::memory_order::seq_cst) noexcept {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, true, __o,
                                                      ::__ycxx::__detail::__atomic_failure_order(__o));
  }
  bool compare_exchange_strong(_Tp& expected, _Tp __desired, std::memory_order __o = std::memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, false, __o,
                                                      ::__ycxx::__detail::__atomic_failure_order(__o));
  }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  bool compare_exchange_strong(_Tp& expected, _Tp __desired, std::memory_order __o = std::memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, false, __o,
                                                      ::__ycxx::__detail::__atomic_failure_order(__o));
  }
  constexpr bool compare_exchange_strong(_Tp& expected, _Tp __desired,
                                         std::memory_order __o = std::memory_order::seq_cst) noexcept {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<_Tp>(__builtin_addressof(__v_), expected, __desired, false, __o,
                                                      ::__ycxx::__detail::__atomic_failure_order(__o));
  }
  void wait(_Tp __old, std::memory_order __o = std::memory_order::seq_cst) const volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::atomic_wait<_Tp>(__builtin_addressof(__v_), __old, __o);
  }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  void wait(_Tp __old, std::memory_order __o = std::memory_order::seq_cst) const volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::atomic_wait<_Tp>(__builtin_addressof(__v_), __old, __o);
  }
  constexpr void wait(_Tp __old, std::memory_order __o = std::memory_order::seq_cst) const noexcept {
    ::__ycxx::__detail::atomic_wait<_Tp>(__builtin_addressof(__v_), __old, __o);
  }
  void notify_one() volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { ::__ycxx::__detail::atomic_notify_all(__builtin_addressof(__v_)); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  void notify_one() volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { ::__ycxx::__detail::atomic_notify_all(__builtin_addressof(__v_)); }
  constexpr void notify_one() noexcept { ::__ycxx::__detail::atomic_notify_all(__builtin_addressof(__v_)); }
  void notify_all() volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { ::__ycxx::__detail::atomic_notify_all(__builtin_addressof(__v_)); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  void notify_all() volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { ::__ycxx::__detail::atomic_notify_all(__builtin_addressof(__v_)); }
  constexpr void notify_all() noexcept { ::__ycxx::__detail::atomic_notify_all(__builtin_addressof(__v_)); }
};

// The pointer and the operations every atomic_ref<T> has.
template <class _Tp>
struct __atomic_ref_base {
  using value_type = std::remove_cv_t<_Tp>;

  _Tp* __ptr_;

  constexpr explicit __atomic_ref_base(_Tp* p) noexcept : __ptr_(p) {}

  static constexpr std::size_t required_alignment = ::__ycxx::__detail::__atomic_align<value_type>;
  static constexpr bool is_always_lock_free = ::__ycxx::__detail::__atomic_lock_free<value_type>;
  static_assert(std::is_trivially_copyable_v<value_type>, "atomic_ref<T> needs a trivially copyable T");
  static_assert(is_always_lock_free || !std::is_volatile_v<_Tp>,
                "atomic_ref<volatile T> needs a T whose operations are always lock-free");

  bool is_lock_free() const noexcept { return is_always_lock_free; }

  constexpr void store(value_type __desired, std::memory_order __o = std::memory_order::seq_cst) const noexcept
    requires(!std::is_const_v<_Tp>)
  {
    ::__ycxx::__detail::atomic_store<value_type>(__ptr_, __desired, __o);
  }
  constexpr value_type operator=(value_type __desired) const noexcept
    requires(!std::is_const_v<_Tp>)
  {
    store(__desired);
    return __desired;
  }
  constexpr value_type load(std::memory_order __o = std::memory_order::seq_cst) const noexcept {
    return ::__ycxx::__detail::atomic_load<value_type>(__ptr_, __o);
  }
  constexpr operator value_type() const noexcept { return load(); }
  constexpr value_type exchange(value_type __desired, std::memory_order __o = std::memory_order::seq_cst) const noexcept
    requires(!std::is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::atomic_exchange<value_type>(__ptr_, __desired, __o);
  }
  constexpr bool compare_exchange_weak(value_type& expected, value_type __desired, std::memory_order s,
                                       std::memory_order __f) const noexcept
    requires(!std::is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<value_type>(__ptr_, expected, __desired, true, s, __f);
  }
  constexpr bool compare_exchange_strong(value_type& expected, value_type __desired, std::memory_order s,
                                         std::memory_order __f) const noexcept
    requires(!std::is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<value_type>(__ptr_, expected, __desired, false, s, __f);
  }
  constexpr bool compare_exchange_weak(value_type& expected, value_type __desired,
                                       std::memory_order __o = std::memory_order::seq_cst) const noexcept
    requires(!std::is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<value_type>(__ptr_, expected, __desired, true, __o,
                                                               ::__ycxx::__detail::__atomic_failure_order(__o));
  }
  constexpr bool compare_exchange_strong(value_type& expected, value_type __desired,
                                         std::memory_order __o = std::memory_order::seq_cst) const noexcept
    requires(!std::is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__y_atomic_compare_exchange<value_type>(__ptr_, expected, __desired, false, __o,
                                                               ::__ycxx::__detail::__atomic_failure_order(__o));
  }
  constexpr void wait(value_type __old, std::memory_order __o = std::memory_order::seq_cst) const noexcept {
    ::__ycxx::__detail::atomic_wait<value_type>(__ptr_, __old, __o);
  }
  constexpr void notify_one() const noexcept
    requires(!std::is_const_v<_Tp>)
  {
    ::__ycxx::__detail::atomic_notify_all(__ptr_);
  }
  constexpr void notify_all() const noexcept
    requires(!std::is_const_v<_Tp>)
  {
    ::__ycxx::__detail::atomic_notify_all(__ptr_);
  }
  constexpr ::__ycxx::__detail::__copy_cv<_Tp, void>* address() const noexcept { return __ptr_; }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- [atomics.ref.generic] ---------------------------------------------------------------------
// The constructors every atomic_ref has; `_Base` is the specialization's base class.
template <class _Tp>
struct atomic_ref : __ycxx::__adl_free::__atomic_ref_base<_Tp> {
  constexpr explicit atomic_ref(_Tp& __obj) : __ycxx::__adl_free::__atomic_ref_base<_Tp>(__builtin_addressof(__obj)) {
    if !consteval {
      __ycxx::__detail::__precondition(reinterpret_cast<__UINTPTR_TYPE__>(this->__ptr_) % this->required_alignment == 0,
                                 "atomic_ref: the object is not aligned to required_alignment");
    }
  }
  explicit atomic_ref(_Tp&&) = delete;
  constexpr atomic_ref(const atomic_ref&) noexcept = default;
  template <class _Up>
    requires __ycxx::__detail::__similar_types<_Tp, _Up> && is_convertible_v<_Up*, _Tp*>
  constexpr atomic_ref(const atomic_ref<_Up>& ref) noexcept
      : __ycxx::__adl_free::__atomic_ref_base<_Tp>(static_cast<_Tp*>(ref.address())) {}
  atomic_ref& operator=(const atomic_ref&) = delete;
  using __ycxx::__adl_free::__atomic_ref_base<_Tp>::operator=;
};

// [atomics.ref.int]
template <class _Tp>
  requires __ycxx::__detail::__atomic_integral<remove_cv_t<_Tp>>
struct atomic_ref<_Tp> : __ycxx::__adl_free::__atomic_ref_base<_Tp> {
  using value_type = remove_cv_t<_Tp>;
  using difference_type = value_type;

  constexpr explicit atomic_ref(_Tp& __obj) : __ycxx::__adl_free::__atomic_ref_base<_Tp>(__builtin_addressof(__obj)) {
    if !consteval {
      __ycxx::__detail::__precondition(reinterpret_cast<__UINTPTR_TYPE__>(this->__ptr_) % this->required_alignment == 0,
                                 "atomic_ref: the object is not aligned to required_alignment");
    }
  }
  explicit atomic_ref(_Tp&&) = delete;
  constexpr atomic_ref(const atomic_ref&) noexcept = default;
  template <class _Up>
    requires __ycxx::__detail::__similar_types<_Tp, _Up> && is_convertible_v<_Up*, _Tp*>
  constexpr atomic_ref(const atomic_ref<_Up>& ref) noexcept
      : __ycxx::__adl_free::__atomic_ref_base<_Tp>(static_cast<_Tp*>(ref.address())) {}
  atomic_ref& operator=(const atomic_ref&) = delete;
  using __ycxx::__adl_free::__atomic_ref_base<_Tp>::operator=;

  constexpr value_type fetch_add(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::add, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_sub(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__sub, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_and(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__and_, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_or(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__or_, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_xor(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__xor_, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_max(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::atomic_fetch_max<value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_min(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::atomic_fetch_min<value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_add(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::add, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_sub(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__sub, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_and(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__and_, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_or(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__or_, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_xor(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__xor_, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_max(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_max<value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_min(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_min<value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type operator++(int) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_add(1);
  }
  constexpr value_type operator--(int) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_sub(1);
  }
  constexpr value_type operator++() const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_wrap_add<value_type>(fetch_add(1), 1);
  }
  constexpr value_type operator--() const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_wrap_add<value_type>(fetch_sub(1), -1);
  }
  constexpr value_type operator+=(value_type a) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_wrap_add<value_type>(fetch_add(a), a);
  }
  constexpr value_type operator-=(value_type a) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_wrap_sub<value_type>(fetch_sub(a), a);
  }
  constexpr value_type operator&=(value_type a) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return static_cast<value_type>(fetch_and(a) & a);
  }
  constexpr value_type operator|=(value_type a) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return static_cast<value_type>(fetch_or(a) | a);
  }
  constexpr value_type operator^=(value_type a) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return static_cast<value_type>(fetch_xor(a) ^ a);
  }
};

// [atomics.ref.float]
template <class _Tp>
  requires __ycxx::__detail::__atomic_floating<remove_cv_t<_Tp>>
struct atomic_ref<_Tp> : __ycxx::__adl_free::__atomic_ref_base<_Tp> {
  using value_type = remove_cv_t<_Tp>;
  using difference_type = value_type;

  constexpr explicit atomic_ref(_Tp& __obj) : __ycxx::__adl_free::__atomic_ref_base<_Tp>(__builtin_addressof(__obj)) {
    if !consteval {
      __ycxx::__detail::__precondition(reinterpret_cast<__UINTPTR_TYPE__>(this->__ptr_) % this->required_alignment == 0,
                                 "atomic_ref: the object is not aligned to required_alignment");
    }
  }
  explicit atomic_ref(_Tp&&) = delete;
  constexpr atomic_ref(const atomic_ref&) noexcept = default;
  template <class _Up>
    requires __ycxx::__detail::__similar_types<_Tp, _Up> && is_convertible_v<_Up*, _Tp*>
  constexpr atomic_ref(const atomic_ref<_Up>& ref) noexcept
      : __ycxx::__adl_free::__atomic_ref_base<_Tp>(static_cast<_Tp*>(ref.address())) {}
  atomic_ref& operator=(const atomic_ref&) = delete;
  using __ycxx::__adl_free::__atomic_ref_base<_Tp>::operator=;

  constexpr value_type fetch_add(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::add, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_sub(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__sub, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_max(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_min(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_fmaximum(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_fminimum(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_fmaximum_num(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_fminimum_num(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_add(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::add, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_sub(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__sub, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_max(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_min(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_fmaximum(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_fminimum(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_fmaximum_num(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_fminimum_num(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type operator+=(value_type a) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return static_cast<value_type>(fetch_add(a) + a);
  }
  constexpr value_type operator-=(value_type a) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return static_cast<value_type>(fetch_sub(a) - a);
  }
};

// [atomics.ref.pointer]
template <class _Tp>
  requires is_pointer_v<remove_cv_t<_Tp>> && is_object_v<remove_pointer_t<remove_cv_t<_Tp>>>
struct atomic_ref<_Tp> : __ycxx::__adl_free::__atomic_ref_base<_Tp> {
  using value_type = remove_cv_t<_Tp>;
  using difference_type = ptrdiff_t;

  constexpr explicit atomic_ref(_Tp& __obj) : __ycxx::__adl_free::__atomic_ref_base<_Tp>(__builtin_addressof(__obj)) {
    if !consteval {
      __ycxx::__detail::__precondition(reinterpret_cast<__UINTPTR_TYPE__>(this->__ptr_) % this->required_alignment == 0,
                                 "atomic_ref: the object is not aligned to required_alignment");
    }
  }
  explicit atomic_ref(_Tp&&) = delete;
  constexpr atomic_ref(const atomic_ref&) noexcept = default;
  template <class _Up>
    requires __ycxx::__detail::__similar_types<_Tp, _Up> && is_convertible_v<_Up*, _Tp*>
  constexpr atomic_ref(const atomic_ref<_Up>& ref) noexcept
      : __ycxx::__adl_free::__atomic_ref_base<_Tp>(static_cast<_Tp*>(ref.address())) {}
  atomic_ref& operator=(const atomic_ref&) = delete;
  using __ycxx::__adl_free::__atomic_ref_base<_Tp>::operator=;

  constexpr value_type fetch_add(ptrdiff_t a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_ptr<value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_sub(ptrdiff_t a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_fetch_ptr<value_type>(this->__ptr_, -a, __o);
  }
  constexpr value_type fetch_max(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::atomic_fetch_max<value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type fetch_min(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::atomic_fetch_min<value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_add(ptrdiff_t a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_ptr<value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_sub(ptrdiff_t a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_ptr<value_type>(this->__ptr_, -a, __o);
  }
  constexpr void store_max(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_max<value_type>(this->__ptr_, a, __o);
  }
  constexpr void store_min(value_type a, memory_order __o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_min<value_type>(this->__ptr_, a, __o);
  }
  constexpr value_type operator++(int) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_add(1);
  }
  constexpr value_type operator--(int) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return fetch_sub(1);
  }
  constexpr value_type operator++() const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_ptr_add<value_type>(fetch_add(1), 1);
  }
  constexpr value_type operator--() const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_ptr_add<value_type>(fetch_sub(1), -1);
  }
  constexpr value_type operator+=(difference_type a) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_ptr_add<value_type>(fetch_add(a), a);
  }
  constexpr value_type operator-=(difference_type a) const noexcept
    requires(!is_const_v<_Tp>)
  {
    return ::__ycxx::__detail::__atomic_ptr_add<value_type>(fetch_sub(a), -a);
  }
};

// ---- [atomics.types.generic] -------------------------------------------------------------------
template <class _Tp>
struct atomic : __ycxx::__adl_free::__atomic_base<_Tp> {
  static_assert(is_trivially_copyable_v<_Tp> && is_copy_constructible_v<_Tp> && is_move_constructible_v<_Tp> &&
                    is_copy_assignable_v<_Tp> && is_move_assignable_v<_Tp>,
                "atomic<T> needs a trivially copyable, copy and move constructible and assignable T");
  static_assert(is_same_v<_Tp, remove_cv_t<_Tp>>, "atomic<T> needs a cv-unqualified T");

  using value_type = _Tp;

  constexpr atomic() noexcept(is_nothrow_default_constructible_v<_Tp>)
    requires is_default_constructible_v<_Tp>
      : __ycxx::__adl_free::__atomic_base<_Tp>() {}
  constexpr atomic(_Tp __desired) noexcept : __ycxx::__adl_free::__atomic_base<_Tp>(__desired) {}
  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;
  atomic& operator=(const atomic&) volatile = delete;
  using __ycxx::__adl_free::__atomic_base<_Tp>::operator=;
};

// [atomics.types.int]
template <class _Tp>
  requires __ycxx::__detail::__atomic_integral<_Tp> && is_same_v<_Tp, remove_cv_t<_Tp>>
struct atomic<_Tp> : __ycxx::__adl_free::__atomic_base<_Tp> {
  using value_type = _Tp;
  using difference_type = value_type;

  constexpr atomic() noexcept : __ycxx::__adl_free::__atomic_base<_Tp>() {}
  constexpr atomic(_Tp __desired) noexcept : __ycxx::__adl_free::__atomic_base<_Tp>(__desired) {}
  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;
  atomic& operator=(const atomic&) volatile = delete;
  using __ycxx::__adl_free::__atomic_base<_Tp>::operator=;

  _Tp fetch_add(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::add, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_add(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::add, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_add(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::add, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_sub(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__sub, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_sub(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__sub, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_sub(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__sub, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_and(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__and_, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_and(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__and_, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_and(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__and_, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_or(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__or_, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_or(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__or_, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_or(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__or_, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_xor(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__xor_, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_xor(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__xor_, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_xor(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__xor_, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_max(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::atomic_fetch_max<_Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_max(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::atomic_fetch_max<_Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_max(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::atomic_fetch_max<_Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_min(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::atomic_fetch_min<_Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_min(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::atomic_fetch_min<_Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_min(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::atomic_fetch_min<_Tp>(__builtin_addressof(this->__v_), a, __o); }
  void store_add(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::add, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_add(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::add, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_sub(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__sub, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_sub(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__sub, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_and(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__and_, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_and(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__and_, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_or(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__or_, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_or(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__or_, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_xor(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__xor_, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_xor(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_int<::__ycxx::__detail::__atomic_int_op::__xor_, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_max(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_max<_Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_max(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_max<_Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_min(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_min<_Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_min(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_min<_Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  _Tp operator++(int) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return fetch_add(1); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator++(int) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return fetch_add(1); }
  _Tp operator--(int) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return fetch_sub(1); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator--(int) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return fetch_sub(1); }
  _Tp operator++() volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_wrap_add<_Tp>(fetch_add(1), 1); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator++() volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_wrap_add<_Tp>(fetch_add(1), 1); }
  _Tp operator--() volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_wrap_add<_Tp>(fetch_sub(1), -1); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator--() volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_wrap_add<_Tp>(fetch_sub(1), -1); }
  constexpr _Tp operator++(int) noexcept { return fetch_add(1); }
  constexpr _Tp operator--(int) noexcept { return fetch_sub(1); }
  constexpr _Tp operator++() noexcept { return ::__ycxx::__detail::__atomic_wrap_add<_Tp>(fetch_add(1), 1); }
  constexpr _Tp operator--() noexcept { return ::__ycxx::__detail::__atomic_wrap_add<_Tp>(fetch_sub(1), -1); }
  _Tp operator+=(_Tp a) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_wrap_add<_Tp>(fetch_add(a), a); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator+=(_Tp a) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_wrap_add<_Tp>(fetch_add(a), a); }
  constexpr _Tp operator+=(_Tp a) noexcept { return ::__ycxx::__detail::__atomic_wrap_add<_Tp>(fetch_add(a), a); }
  _Tp operator-=(_Tp a) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_wrap_sub<_Tp>(fetch_sub(a), a); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator-=(_Tp a) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_wrap_sub<_Tp>(fetch_sub(a), a); }
  constexpr _Tp operator-=(_Tp a) noexcept { return ::__ycxx::__detail::__atomic_wrap_sub<_Tp>(fetch_sub(a), a); }
  _Tp operator&=(_Tp a) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return static_cast<_Tp>(fetch_and(a) & a); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator&=(_Tp a) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return static_cast<_Tp>(fetch_and(a) & a); }
  constexpr _Tp operator&=(_Tp a) noexcept { return static_cast<_Tp>(fetch_and(a) & a); }
  _Tp operator|=(_Tp a) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return static_cast<_Tp>(fetch_or(a) | a); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator|=(_Tp a) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return static_cast<_Tp>(fetch_or(a) | a); }
  constexpr _Tp operator|=(_Tp a) noexcept { return static_cast<_Tp>(fetch_or(a) | a); }
  _Tp operator^=(_Tp a) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return static_cast<_Tp>(fetch_xor(a) ^ a); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator^=(_Tp a) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return static_cast<_Tp>(fetch_xor(a) ^ a); }
  constexpr _Tp operator^=(_Tp a) noexcept { return static_cast<_Tp>(fetch_xor(a) ^ a); }
};

// [atomics.types.float]
template <class _Tp>
  requires __ycxx::__detail::__atomic_floating<_Tp> && is_same_v<_Tp, remove_cv_t<_Tp>>
struct atomic<_Tp> : __ycxx::__adl_free::__atomic_base<_Tp> {
  using value_type = _Tp;
  using difference_type = value_type;

  constexpr atomic() noexcept : __ycxx::__adl_free::__atomic_base<_Tp>() {}
  constexpr atomic(_Tp __desired) noexcept : __ycxx::__adl_free::__atomic_base<_Tp>(__desired) {}
  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;
  atomic& operator=(const atomic&) volatile = delete;
  using __ycxx::__adl_free::__atomic_base<_Tp>::operator=;

  _Tp fetch_add(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::add, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_add(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::add, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_add(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::add, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_sub(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__sub, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_sub(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__sub, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_sub(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__sub, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_max(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_max(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_max(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_min(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_min(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_min(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_fmaximum(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_fmaximum(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_fmaximum(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_fminimum(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_fminimum(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_fminimum(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_fmaximum_num(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_fmaximum_num(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_fmaximum_num(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  _Tp fetch_fminimum_num(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp fetch_fminimum_num(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp fetch_fminimum_num(_Tp a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, _Tp>(__builtin_addressof(this->__v_), a, __o); }
  void store_add(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::add, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_add(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::add, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_sub(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__sub, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_sub(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__sub, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_max(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_max(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_min(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_min(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_fmaximum(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_fmaximum(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_fminimum(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_fminimum(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_fmaximum_num(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_fmaximum_num(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__maximum_num, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_fminimum_num(_Tp a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_fminimum_num(_Tp a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_fp<::__ycxx::__detail::__atomic_fp_op::__minimum_num, _Tp>(__builtin_addressof(this->__v_), a, __o);
  }
  _Tp operator+=(_Tp a) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return static_cast<_Tp>(fetch_add(a) + a); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator+=(_Tp a) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return static_cast<_Tp>(fetch_add(a) + a); }
  constexpr _Tp operator+=(_Tp a) noexcept { return static_cast<_Tp>(fetch_add(a) + a); }
  _Tp operator-=(_Tp a) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return static_cast<_Tp>(fetch_sub(a) - a); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp operator-=(_Tp a) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp>)
  { return static_cast<_Tp>(fetch_sub(a) - a); }
  constexpr _Tp operator-=(_Tp a) noexcept { return static_cast<_Tp>(fetch_sub(a) - a); }
};

// [atomics.types.pointer]
template <class _Tp>
struct atomic<_Tp*> : __ycxx::__adl_free::__atomic_base<_Tp*> {
  using value_type = _Tp*;
  using difference_type = ptrdiff_t;

  constexpr atomic() noexcept : __ycxx::__adl_free::__atomic_base<_Tp*>() {}
  constexpr atomic(_Tp* __desired) noexcept : __ycxx::__adl_free::__atomic_base<_Tp*>(__desired) {}
  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;
  atomic& operator=(const atomic&) volatile = delete;
  using __ycxx::__adl_free::__atomic_base<_Tp*>::operator=;

  _Tp* fetch_add(ptrdiff_t a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_fetch_ptr<_Tp*>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp* fetch_add(ptrdiff_t a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_fetch_ptr<_Tp*>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp* fetch_add(ptrdiff_t a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_ptr<_Tp*>(__builtin_addressof(this->__v_), a, __o); }
  _Tp* fetch_sub(ptrdiff_t a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_fetch_ptr<_Tp*>(__builtin_addressof(this->__v_), -a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp* fetch_sub(ptrdiff_t a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_fetch_ptr<_Tp*>(__builtin_addressof(this->__v_), -a, __o); }
  constexpr _Tp* fetch_sub(ptrdiff_t a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::__atomic_fetch_ptr<_Tp*>(__builtin_addressof(this->__v_), -a, __o); }
  _Tp* fetch_max(_Tp* a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::atomic_fetch_max<_Tp*>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp* fetch_max(_Tp* a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::atomic_fetch_max<_Tp*>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp* fetch_max(_Tp* a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::atomic_fetch_max<_Tp*>(__builtin_addressof(this->__v_), a, __o); }
  _Tp* fetch_min(_Tp* a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::atomic_fetch_min<_Tp*>(__builtin_addressof(this->__v_), a, __o); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp* fetch_min(_Tp* a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::atomic_fetch_min<_Tp*>(__builtin_addressof(this->__v_), a, __o); }
  constexpr _Tp* fetch_min(_Tp* a, memory_order __o = memory_order::seq_cst) noexcept { return ::__ycxx::__detail::atomic_fetch_min<_Tp*>(__builtin_addressof(this->__v_), a, __o); }
  void store_add(ptrdiff_t a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_ptr<_Tp*>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_add(ptrdiff_t a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_ptr<_Tp*>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_sub(ptrdiff_t a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_ptr<_Tp*>(__builtin_addressof(this->__v_), -a, __o);
  }
  constexpr void store_sub(ptrdiff_t a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::__atomic_fetch_ptr<_Tp*>(__builtin_addressof(this->__v_), -a, __o);
  }
  void store_max(_Tp* a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_max<_Tp*>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_max(_Tp* a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_max<_Tp*>(__builtin_addressof(this->__v_), a, __o);
  }
  void store_min(_Tp* a, memory_order __o = memory_order::seq_cst) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_min<_Tp*>(__builtin_addressof(this->__v_), a, __o);
  }
  constexpr void store_min(_Tp* a, memory_order __o = memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    (void)::__ycxx::__detail::atomic_fetch_min<_Tp*>(__builtin_addressof(this->__v_), a, __o);
  }
  _Tp* operator++(int) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return fetch_add(1); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp* operator++(int) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return fetch_add(1); }
  _Tp* operator--(int) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return fetch_sub(1); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp* operator--(int) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return fetch_sub(1); }
  _Tp* operator++() volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_add(1), 1); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp* operator++() volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_add(1), 1); }
  _Tp* operator--() volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_sub(1), -1); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp* operator--() volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_sub(1), -1); }
  constexpr _Tp* operator++(int) noexcept { return fetch_add(1); }
  constexpr _Tp* operator--(int) noexcept { return fetch_sub(1); }
  constexpr _Tp* operator++() noexcept { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_add(1), 1); }
  constexpr _Tp* operator--() noexcept { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_sub(1), -1); }
  _Tp* operator+=(ptrdiff_t a) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_add(a), a); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp* operator+=(ptrdiff_t a) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_add(a), a); }
  constexpr _Tp* operator+=(ptrdiff_t a) noexcept { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_add(a), a); }
  _Tp* operator-=(ptrdiff_t a) volatile noexcept
    requires(::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_sub(a), -a); }
  [[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
               "([depr.atomics.volatile])")]]
  _Tp* operator-=(ptrdiff_t a) volatile noexcept
    requires(!::__ycxx::__detail::__atomic_lock_free<_Tp*>)
  { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_sub(a), -a); }
  constexpr _Tp* operator-=(ptrdiff_t a) noexcept { return ::__ycxx::__detail::__atomic_ptr_add<_Tp*>(fetch_sub(a), -a); }
};

// ---- [atomics.flag] ----------------------------------------------------------------------------
struct atomic_flag {
private:
  unsigned char __v_;

public:
  constexpr atomic_flag() noexcept : __v_(0) {}
  atomic_flag(const atomic_flag&) = delete;
  atomic_flag& operator=(const atomic_flag&) = delete;
  atomic_flag& operator=(const atomic_flag&) volatile = delete;

  bool test(memory_order __o = memory_order::seq_cst) const volatile noexcept {
    return __ycxx::__detail::atomic_load<unsigned char>(&__v_, __o) != 0;
  }
  constexpr bool test(memory_order __o = memory_order::seq_cst) const noexcept {
    return __ycxx::__detail::atomic_load<unsigned char>(&__v_, __o) != 0;
  }
  bool test_and_set(memory_order __o = memory_order::seq_cst) volatile noexcept {
    return __ycxx::__detail::atomic_exchange<unsigned char>(&__v_, 1, __o) != 0;
  }
  constexpr bool test_and_set(memory_order __o = memory_order::seq_cst) noexcept {
    return __ycxx::__detail::atomic_exchange<unsigned char>(&__v_, 1, __o) != 0;
  }
  void clear(memory_order __o = memory_order::seq_cst) volatile noexcept {
    __ycxx::__detail::atomic_store<unsigned char>(&__v_, 0, __o);
  }
  constexpr void clear(memory_order __o = memory_order::seq_cst) noexcept {
    __ycxx::__detail::atomic_store<unsigned char>(&__v_, 0, __o);
  }
  void wait(bool __old, memory_order __o = memory_order::seq_cst) const volatile noexcept {
    __ycxx::__detail::atomic_wait<unsigned char>(&__v_, __old ? 1 : 0, __o);
  }
  constexpr void wait(bool __old, memory_order __o = memory_order::seq_cst) const noexcept {
    __ycxx::__detail::atomic_wait<unsigned char>(&__v_, __old ? 1 : 0, __o);
  }
  void notify_one() volatile noexcept { __ycxx::__detail::atomic_notify_all(&__v_); }
  constexpr void notify_one() noexcept { __ycxx::__detail::atomic_notify_all(&__v_); }
  void notify_all() volatile noexcept { __ycxx::__detail::atomic_notify_all(&__v_); }
  constexpr void notify_all() noexcept { __ycxx::__detail::atomic_notify_all(&__v_); }
};

inline bool atomic_flag_test(const volatile atomic_flag* __object) noexcept { return __object->test(); }
constexpr bool atomic_flag_test(const atomic_flag* __object) noexcept { return __object->test(); }
inline bool atomic_flag_test_explicit(const volatile atomic_flag* __object, memory_order __o) noexcept {
  return __object->test(__o);
}
constexpr bool atomic_flag_test_explicit(const atomic_flag* __object, memory_order __o) noexcept { return __object->test(__o); }
inline bool atomic_flag_test_and_set(volatile atomic_flag* __object) noexcept { return __object->test_and_set(); }
constexpr bool atomic_flag_test_and_set(atomic_flag* __object) noexcept { return __object->test_and_set(); }
inline bool atomic_flag_test_and_set_explicit(volatile atomic_flag* __object, memory_order __o) noexcept {
  return __object->test_and_set(__o);
}
constexpr bool atomic_flag_test_and_set_explicit(atomic_flag* __object, memory_order __o) noexcept {
  return __object->test_and_set(__o);
}
inline void atomic_flag_clear(volatile atomic_flag* __object) noexcept { __object->clear(); }
constexpr void atomic_flag_clear(atomic_flag* __object) noexcept { __object->clear(); }
inline void atomic_flag_clear_explicit(volatile atomic_flag* __object, memory_order __o) noexcept { __object->clear(__o); }
constexpr void atomic_flag_clear_explicit(atomic_flag* __object, memory_order __o) noexcept { __object->clear(__o); }
inline void atomic_flag_wait(const volatile atomic_flag* __object, bool __old) noexcept { __object->wait(__old); }
constexpr void atomic_flag_wait(const atomic_flag* __object, bool __old) noexcept { __object->wait(__old); }
inline void atomic_flag_wait_explicit(const volatile atomic_flag* __object, bool __old, memory_order __o) noexcept {
  __object->wait(__old, __o);
}
constexpr void atomic_flag_wait_explicit(const atomic_flag* __object, bool __old, memory_order __o) noexcept {
  __object->wait(__old, __o);
}
inline void atomic_flag_notify_one(volatile atomic_flag* __object) noexcept { __object->notify_one(); }
constexpr void atomic_flag_notify_one(atomic_flag* __object) noexcept { __object->notify_one(); }
inline void atomic_flag_notify_all(volatile atomic_flag* __object) noexcept { __object->notify_all(); }
constexpr void atomic_flag_notify_all(atomic_flag* __object) noexcept { __object->notify_all(); }

// ---- [atomics.fences] --------------------------------------------------------------------------
extern "C" constexpr void atomic_thread_fence(memory_order __order) noexcept {
  if !consteval {
    __atomic_thread_fence(static_cast<int>(__order));
  }
}
extern "C" constexpr void atomic_signal_fence(memory_order __order) noexcept {
  if !consteval {
    __atomic_signal_fence(static_cast<int>(__order));
  }
}

// ---- [atomics.alias] ---------------------------------------------------------------------------
using atomic_bool = atomic<bool>;
using atomic_char = atomic<char>;
using atomic_schar = atomic<signed char>;
using atomic_uchar = atomic<unsigned char>;
using atomic_short = atomic<short>;
using atomic_ushort = atomic<unsigned short>;
using atomic_int = atomic<int>;
using atomic_uint = atomic<unsigned int>;
using atomic_long = atomic<long>;
using atomic_ulong = atomic<unsigned long>;
using atomic_llong = atomic<long long>;
using atomic_ullong = atomic<unsigned long long>;
using atomic_char8_t = atomic<char8_t>;
using atomic_char16_t = atomic<char16_t>;
using atomic_char32_t = atomic<char32_t>;
using atomic_wchar_t = atomic<wchar_t>;
using atomic_int8_t = atomic<int8_t>;
using atomic_uint8_t = atomic<uint8_t>;
using atomic_int16_t = atomic<int16_t>;
using atomic_uint16_t = atomic<uint16_t>;
using atomic_int32_t = atomic<int32_t>;
using atomic_uint32_t = atomic<uint32_t>;
using atomic_int64_t = atomic<int64_t>;
using atomic_uint64_t = atomic<uint64_t>;
using atomic_int_least8_t = atomic<int_least8_t>;
using atomic_uint_least8_t = atomic<uint_least8_t>;
using atomic_int_least16_t = atomic<int_least16_t>;
using atomic_uint_least16_t = atomic<uint_least16_t>;
using atomic_int_least32_t = atomic<int_least32_t>;
using atomic_uint_least32_t = atomic<uint_least32_t>;
using atomic_int_least64_t = atomic<int_least64_t>;
using atomic_uint_least64_t = atomic<uint_least64_t>;
using atomic_int_fast8_t = atomic<int_fast8_t>;
using atomic_uint_fast8_t = atomic<uint_fast8_t>;
using atomic_int_fast16_t = atomic<int_fast16_t>;
using atomic_uint_fast16_t = atomic<uint_fast16_t>;
using atomic_int_fast32_t = atomic<int_fast32_t>;
using atomic_uint_fast32_t = atomic<uint_fast32_t>;
using atomic_int_fast64_t = atomic<int_fast64_t>;
using atomic_uint_fast64_t = atomic<uint_fast64_t>;
using atomic_intptr_t = atomic<intptr_t>;
using atomic_uintptr_t = atomic<uintptr_t>;
using atomic_size_t = atomic<size_t>;
using atomic_ptrdiff_t = atomic<ptrdiff_t>;
using atomic_intmax_t = atomic<intmax_t>;
using atomic_uintmax_t = atomic<uintmax_t>;
// The waiting operations cost the same for every size (they go through the slot table); int
// is lock-free on every target libycxx supports.
using atomic_signed_lock_free = atomic<int>;
using atomic_unsigned_lock_free = atomic<unsigned int>;
static_assert(atomic_signed_lock_free::is_always_lock_free && atomic_unsigned_lock_free::is_always_lock_free);

// ---- [atomics.nonmembers] ----------------------------------------------------------------------
// A volatile overload calls the volatile member; for a T that is not always lock-free that member
// is Annex D ([depr.atomics.volatile]), so the overload is split the same way as the member. The
// volatile store_key members exist only for always lock-free types ([atomics.types.int]), and so
// do their non-member functions.
template <class _Tp>
bool atomic_is_lock_free(const volatile atomic<_Tp>* __object) noexcept {
  return __object->is_lock_free();
}
template <class _Tp>
bool atomic_is_lock_free(const atomic<_Tp>* __object) noexcept {
  return __object->is_lock_free();
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired) noexcept {
  __object->store(__desired);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
void atomic_store(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired) noexcept {
  __object->store(__desired);
}
template <class _Tp>
constexpr void atomic_store(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired) noexcept {
  __object->store(__desired);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired, memory_order __o) noexcept {
  __object->store(__desired, __o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
void atomic_store_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired, memory_order __o) noexcept {
  __object->store(__desired, __o);
}
template <class _Tp>
constexpr void atomic_store_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired, memory_order __o) noexcept {
  __object->store(__desired, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_load(const volatile atomic<_Tp>* __object) noexcept {
  return __object->load();
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_load(const volatile atomic<_Tp>* __object) noexcept {
  return __object->load();
}
template <class _Tp>
constexpr _Tp atomic_load(const atomic<_Tp>* __object) noexcept {
  return __object->load();
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_load_explicit(const volatile atomic<_Tp>* __object, memory_order __o) noexcept {
  return __object->load(__o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_load_explicit(const volatile atomic<_Tp>* __object, memory_order __o) noexcept {
  return __object->load(__o);
}
template <class _Tp>
constexpr _Tp atomic_load_explicit(const atomic<_Tp>* __object, memory_order __o) noexcept {
  return __object->load(__o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_exchange(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired) noexcept {
  return __object->exchange(__desired);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_exchange(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired) noexcept {
  return __object->exchange(__desired);
}
template <class _Tp>
constexpr _Tp atomic_exchange(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired) noexcept {
  return __object->exchange(__desired);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_exchange_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired, memory_order __o) noexcept {
  return __object->exchange(__desired, __o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_exchange_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired, memory_order __o) noexcept {
  return __object->exchange(__desired, __o);
}
template <class _Tp>
constexpr _Tp atomic_exchange_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired, memory_order __o) noexcept {
  return __object->exchange(__desired, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
bool atomic_compare_exchange_weak(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                  typename atomic<_Tp>::value_type __desired) noexcept {
  return __object->compare_exchange_weak(*expected, __desired);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
bool atomic_compare_exchange_weak(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                  typename atomic<_Tp>::value_type __desired) noexcept {
  return __object->compare_exchange_weak(*expected, __desired);
}
template <class _Tp>
constexpr bool atomic_compare_exchange_weak(atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                            typename atomic<_Tp>::value_type __desired) noexcept {
  return __object->compare_exchange_weak(*expected, __desired);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
bool atomic_compare_exchange_strong(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                    typename atomic<_Tp>::value_type __desired) noexcept {
  return __object->compare_exchange_strong(*expected, __desired);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
bool atomic_compare_exchange_strong(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                    typename atomic<_Tp>::value_type __desired) noexcept {
  return __object->compare_exchange_strong(*expected, __desired);
}
template <class _Tp>
constexpr bool atomic_compare_exchange_strong(atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                              typename atomic<_Tp>::value_type __desired) noexcept {
  return __object->compare_exchange_strong(*expected, __desired);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
bool atomic_compare_exchange_weak_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                           typename atomic<_Tp>::value_type __desired, memory_order s,
                                           memory_order __f) noexcept {
  return __object->compare_exchange_weak(*expected, __desired, s, __f);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
bool atomic_compare_exchange_weak_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                           typename atomic<_Tp>::value_type __desired, memory_order s,
                                           memory_order __f) noexcept {
  return __object->compare_exchange_weak(*expected, __desired, s, __f);
}
template <class _Tp>
constexpr bool atomic_compare_exchange_weak_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                                     typename atomic<_Tp>::value_type __desired, memory_order s,
                                                     memory_order __f) noexcept {
  return __object->compare_exchange_weak(*expected, __desired, s, __f);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
bool atomic_compare_exchange_strong_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                             typename atomic<_Tp>::value_type __desired, memory_order s,
                                             memory_order __f) noexcept {
  return __object->compare_exchange_strong(*expected, __desired, s, __f);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
bool atomic_compare_exchange_strong_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                             typename atomic<_Tp>::value_type __desired, memory_order s,
                                             memory_order __f) noexcept {
  return __object->compare_exchange_strong(*expected, __desired, s, __f);
}
template <class _Tp>
constexpr bool atomic_compare_exchange_strong_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type* expected,
                                                       typename atomic<_Tp>::value_type __desired, memory_order s,
                                                       memory_order __f) noexcept {
  return __object->compare_exchange_strong(*expected, __desired, s, __f);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_add(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand) noexcept {
  return __object->fetch_add(__operand);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_add(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand) noexcept {
  return __object->fetch_add(__operand);
}
template <class _Tp>
constexpr _Tp atomic_fetch_add(atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand) noexcept {
  return __object->fetch_add(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_add_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand, memory_order __o) noexcept {
  return __object->fetch_add(__operand, __o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_add_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand, memory_order __o) noexcept {
  return __object->fetch_add(__operand, __o);
}
template <class _Tp>
constexpr _Tp atomic_fetch_add_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand, memory_order __o) noexcept {
  return __object->fetch_add(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_sub(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand) noexcept {
  return __object->fetch_sub(__operand);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_sub(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand) noexcept {
  return __object->fetch_sub(__operand);
}
template <class _Tp>
constexpr _Tp atomic_fetch_sub(atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand) noexcept {
  return __object->fetch_sub(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_sub_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand, memory_order __o) noexcept {
  return __object->fetch_sub(__operand, __o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_sub_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand, memory_order __o) noexcept {
  return __object->fetch_sub(__operand, __o);
}
template <class _Tp>
constexpr _Tp atomic_fetch_sub_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand, memory_order __o) noexcept {
  return __object->fetch_sub(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_and(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_and(__operand);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_and(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_and(__operand);
}
template <class _Tp>
constexpr _Tp atomic_fetch_and(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_and(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_and_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_and(__operand, __o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_and_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_and(__operand, __o);
}
template <class _Tp>
constexpr _Tp atomic_fetch_and_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_and(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_or(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_or(__operand);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_or(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_or(__operand);
}
template <class _Tp>
constexpr _Tp atomic_fetch_or(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_or(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_or_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_or(__operand, __o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_or_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_or(__operand, __o);
}
template <class _Tp>
constexpr _Tp atomic_fetch_or_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_or(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_xor(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_xor(__operand);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_xor(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_xor(__operand);
}
template <class _Tp>
constexpr _Tp atomic_fetch_xor(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_xor(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_xor_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_xor(__operand, __o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_xor_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_xor(__operand, __o);
}
template <class _Tp>
constexpr _Tp atomic_fetch_xor_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_xor(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_max(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_max(__operand);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_max(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_max(__operand);
}
template <class _Tp>
constexpr _Tp atomic_fetch_max(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_max(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_max_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_max(__operand, __o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_max_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_max(__operand, __o);
}
template <class _Tp>
constexpr _Tp atomic_fetch_max_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_max(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_min(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_min(__operand);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_min(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_min(__operand);
}
template <class _Tp>
constexpr _Tp atomic_fetch_min(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  return __object->fetch_min(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
_Tp atomic_fetch_min_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_min(__operand, __o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
_Tp atomic_fetch_min_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_min(__operand, __o);
}
template <class _Tp>
constexpr _Tp atomic_fetch_min_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  return __object->fetch_min(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_add(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand) noexcept {
  __object->store_add(__operand);
}
template <class _Tp>
constexpr void atomic_store_add(atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand) noexcept {
  __object->store_add(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_add_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand, memory_order __o) noexcept {
  __object->store_add(__operand, __o);
}
template <class _Tp>
constexpr void atomic_store_add_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand, memory_order __o) noexcept {
  __object->store_add(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_sub(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand) noexcept {
  __object->store_sub(__operand);
}
template <class _Tp>
constexpr void atomic_store_sub(atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand) noexcept {
  __object->store_sub(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_sub_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand, memory_order __o) noexcept {
  __object->store_sub(__operand, __o);
}
template <class _Tp>
constexpr void atomic_store_sub_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::difference_type __operand, memory_order __o) noexcept {
  __object->store_sub(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_and(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  __object->store_and(__operand);
}
template <class _Tp>
constexpr void atomic_store_and(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  __object->store_and(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_and_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  __object->store_and(__operand, __o);
}
template <class _Tp>
constexpr void atomic_store_and_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  __object->store_and(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_or(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  __object->store_or(__operand);
}
template <class _Tp>
constexpr void atomic_store_or(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  __object->store_or(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_or_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  __object->store_or(__operand, __o);
}
template <class _Tp>
constexpr void atomic_store_or_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  __object->store_or(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_xor(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  __object->store_xor(__operand);
}
template <class _Tp>
constexpr void atomic_store_xor(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  __object->store_xor(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_xor_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  __object->store_xor(__operand, __o);
}
template <class _Tp>
constexpr void atomic_store_xor_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  __object->store_xor(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_max(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  __object->store_max(__operand);
}
template <class _Tp>
constexpr void atomic_store_max(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  __object->store_max(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_max_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  __object->store_max(__operand, __o);
}
template <class _Tp>
constexpr void atomic_store_max_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  __object->store_max(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_min(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  __object->store_min(__operand);
}
template <class _Tp>
constexpr void atomic_store_min(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand) noexcept {
  __object->store_min(__operand);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_store_min_explicit(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  __object->store_min(__operand, __o);
}
template <class _Tp>
constexpr void atomic_store_min_explicit(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __operand, memory_order __o) noexcept {
  __object->store_min(__operand, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_wait(const volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __old) noexcept {
  __object->wait(__old);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
void atomic_wait(const volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __old) noexcept {
  __object->wait(__old);
}
template <class _Tp>
constexpr void atomic_wait(const atomic<_Tp>* __object, typename atomic<_Tp>::value_type __old) noexcept {
  __object->wait(__old);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_wait_explicit(const volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __old, memory_order __o) noexcept {
  __object->wait(__old, __o);
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
void atomic_wait_explicit(const volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __old, memory_order __o) noexcept {
  __object->wait(__old, __o);
}
template <class _Tp>
constexpr void atomic_wait_explicit(const atomic<_Tp>* __object, typename atomic<_Tp>::value_type __old, memory_order __o) noexcept {
  __object->wait(__old, __o);
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_notify_one(volatile atomic<_Tp>* __object) noexcept {
  __object->notify_one();
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
void atomic_notify_one(volatile atomic<_Tp>* __object) noexcept {
  __object->notify_one();
}
template <class _Tp>
constexpr void atomic_notify_one(atomic<_Tp>* __object) noexcept {
  __object->notify_one();
}
template <class _Tp>
  requires atomic<_Tp>::is_always_lock_free
void atomic_notify_all(volatile atomic<_Tp>* __object) noexcept {
  __object->notify_all();
}
template <class _Tp>
  requires(!atomic<_Tp>::is_always_lock_free)
[[deprecated("a volatile atomic operation on a type that is not always lock-free is deprecated "
             "([depr.atomics.volatile])")]]
void atomic_notify_all(volatile atomic<_Tp>* __object) noexcept {
  __object->notify_all();
}
template <class _Tp>
constexpr void atomic_notify_all(atomic<_Tp>* __object) noexcept {
  __object->notify_all();
}

// [depr.atomics.nonmembers]
template <class _Tp>
[[deprecated("atomic_init is deprecated ([depr.atomics.nonmembers]); use store(desired, memory_order::relaxed)")]]
void atomic_init(volatile atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired) noexcept {
  __object->store(__desired, memory_order::relaxed);
}
template <class _Tp>
[[deprecated("atomic_init is deprecated ([depr.atomics.nonmembers]); use store(desired, memory_order::relaxed)")]]
void atomic_init(atomic<_Tp>* __object, typename atomic<_Tp>::value_type __desired) noexcept {
  __object->store(__desired, memory_order::relaxed);
}

} // namespace std
