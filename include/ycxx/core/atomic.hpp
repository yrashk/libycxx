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

namespace ycxx::detail {

template <class V>
concept atomic_integral = is_integral_v<V> && !std::is_same_v<V, bool>;
template <class V>
concept atomic_floating = is_floating_v<V>;

// x + n / x - n modulo 2^N, for the return values of ++, --, +=, -= ([atomics.types.int]/8).
template <class V>
constexpr V atomic_wrap_add(V x, V n) noexcept {
  using U = std::make_unsigned_t<V>;
  return static_cast<V>(static_cast<U>(static_cast<U>(x) + static_cast<U>(n)));
}
template <class V>
constexpr V atomic_wrap_sub(V x, V n) noexcept {
  using U = std::make_unsigned_t<V>;
  return static_cast<V>(static_cast<U>(static_cast<U>(x) - static_cast<U>(n)));
}
// p + n elements; at run time computed on the integer value, so that an out-of-range result is
// the pointer of that address ([atomics.types.pointer]/2) rather than undefined.
template <class V>
constexpr V atomic_ptr_add(V p, std::ptrdiff_t n) noexcept {
  if consteval {
    return p + n;
  } else {
    using E = std::remove_pointer_t<V>;
    return reinterpret_cast<V>(reinterpret_cast<__UINTPTR_TYPE__>(p) +
                               static_cast<__UINTPTR_TYPE__>(n) * static_cast<__UINTPTR_TYPE__>(sizeof(E)));
  }
}

} // namespace ycxx::detail

namespace ycxx::adl_free {

// The storage and the operations every atomic<T> has.
template <class T>
struct atomic_base {
  alignas(::ycxx::detail::atomic_align<T>) T v_;

  constexpr atomic_base() noexcept(std::is_nothrow_default_constructible_v<T>) : v_() { clear(); }
  constexpr atomic_base(T desired) noexcept : v_(desired) { clear(); }
  atomic_base(const atomic_base&) = delete;
  atomic_base& operator=(const atomic_base&) = delete;
  atomic_base& operator=(const atomic_base&) volatile = delete;

  // Stored values have zero padding bits, so compare-and-exchange rarely needs its retry.
  constexpr void clear() noexcept {
    if !consteval {
      if constexpr (::ycxx::detail::atomic_padded<T>)
        __builtin_clear_padding(__builtin_addressof(v_));
    }
  }

  static constexpr bool is_always_lock_free = ::ycxx::detail::atomic_lock_free<T>;
  bool is_lock_free() const volatile noexcept { return is_always_lock_free; }
  bool is_lock_free() const noexcept { return is_always_lock_free; }

  T load(std::memory_order o = std::memory_order::seq_cst) const volatile noexcept {
    return ::ycxx::detail::atomic_load<T>(__builtin_addressof(v_), o);
  }
  constexpr T load(std::memory_order o = std::memory_order::seq_cst) const noexcept {
    return ::ycxx::detail::atomic_load<T>(__builtin_addressof(v_), o);
  }
  operator T() const volatile noexcept { return load(); }
  constexpr operator T() const noexcept { return load(); }
  void store(T desired, std::memory_order o = std::memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_store<T>(__builtin_addressof(v_), desired, o);
  }
  constexpr void store(T desired, std::memory_order o = std::memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_store<T>(__builtin_addressof(v_), desired, o);
  }
  T operator=(T desired) volatile noexcept {
    store(desired);
    return desired;
  }
  constexpr T operator=(T desired) noexcept {
    store(desired);
    return desired;
  }
  T exchange(T desired, std::memory_order o = std::memory_order::seq_cst) volatile noexcept {
    return ::ycxx::detail::atomic_exchange<T>(__builtin_addressof(v_), desired, o);
  }
  constexpr T exchange(T desired, std::memory_order o = std::memory_order::seq_cst) noexcept {
    return ::ycxx::detail::atomic_exchange<T>(__builtin_addressof(v_), desired, o);
  }
  bool compare_exchange_weak(T& expected, T desired, std::memory_order s, std::memory_order f) volatile noexcept {
    return ::ycxx::detail::atomic_compare_exchange<T>(__builtin_addressof(v_), expected, desired, true, s, f);
  }
  constexpr bool compare_exchange_weak(T& expected, T desired, std::memory_order s, std::memory_order f) noexcept {
    return ::ycxx::detail::atomic_compare_exchange<T>(__builtin_addressof(v_), expected, desired, true, s, f);
  }
  bool compare_exchange_strong(T& expected, T desired, std::memory_order s, std::memory_order f) volatile noexcept {
    return ::ycxx::detail::atomic_compare_exchange<T>(__builtin_addressof(v_), expected, desired, false, s, f);
  }
  constexpr bool compare_exchange_strong(T& expected, T desired, std::memory_order s, std::memory_order f) noexcept {
    return ::ycxx::detail::atomic_compare_exchange<T>(__builtin_addressof(v_), expected, desired, false, s, f);
  }
  bool compare_exchange_weak(T& expected, T desired, std::memory_order o = std::memory_order::seq_cst) volatile noexcept {
    return ::ycxx::detail::atomic_compare_exchange<T>(__builtin_addressof(v_), expected, desired, true, o,
                                                      ::ycxx::detail::atomic_failure_order(o));
  }
  constexpr bool compare_exchange_weak(T& expected, T desired, std::memory_order o = std::memory_order::seq_cst) noexcept {
    return ::ycxx::detail::atomic_compare_exchange<T>(__builtin_addressof(v_), expected, desired, true, o,
                                                      ::ycxx::detail::atomic_failure_order(o));
  }
  bool compare_exchange_strong(T& expected, T desired, std::memory_order o = std::memory_order::seq_cst) volatile noexcept {
    return ::ycxx::detail::atomic_compare_exchange<T>(__builtin_addressof(v_), expected, desired, false, o,
                                                      ::ycxx::detail::atomic_failure_order(o));
  }
  constexpr bool compare_exchange_strong(T& expected, T desired,
                                         std::memory_order o = std::memory_order::seq_cst) noexcept {
    return ::ycxx::detail::atomic_compare_exchange<T>(__builtin_addressof(v_), expected, desired, false, o,
                                                      ::ycxx::detail::atomic_failure_order(o));
  }
  void wait(T old, std::memory_order o = std::memory_order::seq_cst) const volatile noexcept {
    ::ycxx::detail::atomic_wait<T>(__builtin_addressof(v_), old, o);
  }
  constexpr void wait(T old, std::memory_order o = std::memory_order::seq_cst) const noexcept {
    ::ycxx::detail::atomic_wait<T>(__builtin_addressof(v_), old, o);
  }
  void notify_one() volatile noexcept { ::ycxx::detail::atomic_notify_all(__builtin_addressof(v_)); }
  constexpr void notify_one() noexcept { ::ycxx::detail::atomic_notify_all(__builtin_addressof(v_)); }
  void notify_all() volatile noexcept { ::ycxx::detail::atomic_notify_all(__builtin_addressof(v_)); }
  constexpr void notify_all() noexcept { ::ycxx::detail::atomic_notify_all(__builtin_addressof(v_)); }
};

// The pointer and the operations every atomic_ref<T> has.
template <class T>
struct atomic_ref_base {
  using value_type = std::remove_cv_t<T>;

  T* ptr_;

  constexpr explicit atomic_ref_base(T* p) noexcept : ptr_(p) {}

  static constexpr std::size_t required_alignment = ::ycxx::detail::atomic_align<value_type>;
  static constexpr bool is_always_lock_free = ::ycxx::detail::atomic_lock_free<value_type>;
  static_assert(std::is_trivially_copyable_v<value_type>, "atomic_ref<T> needs a trivially copyable T");
  static_assert(is_always_lock_free || !std::is_volatile_v<T>,
                "atomic_ref<volatile T> needs a T whose operations are always lock-free");

  bool is_lock_free() const noexcept { return is_always_lock_free; }

  constexpr void store(value_type desired, std::memory_order o = std::memory_order::seq_cst) const noexcept
    requires(!std::is_const_v<T>)
  {
    ::ycxx::detail::atomic_store<value_type>(ptr_, desired, o);
  }
  constexpr value_type operator=(value_type desired) const noexcept
    requires(!std::is_const_v<T>)
  {
    store(desired);
    return desired;
  }
  constexpr value_type load(std::memory_order o = std::memory_order::seq_cst) const noexcept {
    return ::ycxx::detail::atomic_load<value_type>(ptr_, o);
  }
  constexpr operator value_type() const noexcept { return load(); }
  constexpr value_type exchange(value_type desired, std::memory_order o = std::memory_order::seq_cst) const noexcept
    requires(!std::is_const_v<T>)
  {
    return ::ycxx::detail::atomic_exchange<value_type>(ptr_, desired, o);
  }
  constexpr bool compare_exchange_weak(value_type& expected, value_type desired, std::memory_order s,
                                       std::memory_order f) const noexcept
    requires(!std::is_const_v<T>)
  {
    return ::ycxx::detail::atomic_compare_exchange<value_type>(ptr_, expected, desired, true, s, f);
  }
  constexpr bool compare_exchange_strong(value_type& expected, value_type desired, std::memory_order s,
                                         std::memory_order f) const noexcept
    requires(!std::is_const_v<T>)
  {
    return ::ycxx::detail::atomic_compare_exchange<value_type>(ptr_, expected, desired, false, s, f);
  }
  constexpr bool compare_exchange_weak(value_type& expected, value_type desired,
                                       std::memory_order o = std::memory_order::seq_cst) const noexcept
    requires(!std::is_const_v<T>)
  {
    return ::ycxx::detail::atomic_compare_exchange<value_type>(ptr_, expected, desired, true, o,
                                                               ::ycxx::detail::atomic_failure_order(o));
  }
  constexpr bool compare_exchange_strong(value_type& expected, value_type desired,
                                         std::memory_order o = std::memory_order::seq_cst) const noexcept
    requires(!std::is_const_v<T>)
  {
    return ::ycxx::detail::atomic_compare_exchange<value_type>(ptr_, expected, desired, false, o,
                                                               ::ycxx::detail::atomic_failure_order(o));
  }
  constexpr void wait(value_type old, std::memory_order o = std::memory_order::seq_cst) const noexcept {
    ::ycxx::detail::atomic_wait<value_type>(ptr_, old, o);
  }
  constexpr void notify_one() const noexcept
    requires(!std::is_const_v<T>)
  {
    ::ycxx::detail::atomic_notify_all(ptr_);
  }
  constexpr void notify_all() const noexcept
    requires(!std::is_const_v<T>)
  {
    ::ycxx::detail::atomic_notify_all(ptr_);
  }
  constexpr ::ycxx::detail::copy_cv<T, void>* address() const noexcept { return ptr_; }
};

} // namespace ycxx::adl_free

namespace std {

// ---- [atomics.ref.generic] ---------------------------------------------------------------------
// The constructors every atomic_ref has; `Base` is the specialization's base class.
template <class T>
struct atomic_ref : ycxx::adl_free::atomic_ref_base<T> {
  constexpr explicit atomic_ref(T& obj) : ycxx::adl_free::atomic_ref_base<T>(__builtin_addressof(obj)) {
    if !consteval {
      ycxx::detail::precondition(reinterpret_cast<__UINTPTR_TYPE__>(this->ptr_) % this->required_alignment == 0,
                                 "atomic_ref: the object is not aligned to required_alignment");
    }
  }
  explicit atomic_ref(T&&) = delete;
  constexpr atomic_ref(const atomic_ref&) noexcept = default;
  template <class U>
    requires is_same_v<remove_cv_t<U>, remove_cv_t<T>> && is_convertible_v<U*, T*>
  constexpr atomic_ref(const atomic_ref<U>& ref) noexcept
      : ycxx::adl_free::atomic_ref_base<T>(static_cast<T*>(ref.address())) {}
  atomic_ref& operator=(const atomic_ref&) = delete;
  using ycxx::adl_free::atomic_ref_base<T>::operator=;
};

// [atomics.ref.int]
template <class T>
  requires ycxx::detail::atomic_integral<remove_cv_t<T>>
struct atomic_ref<T> : ycxx::adl_free::atomic_ref_base<T> {
  using value_type = remove_cv_t<T>;
  using difference_type = value_type;

  constexpr explicit atomic_ref(T& obj) : ycxx::adl_free::atomic_ref_base<T>(__builtin_addressof(obj)) {
    if !consteval {
      ycxx::detail::precondition(reinterpret_cast<__UINTPTR_TYPE__>(this->ptr_) % this->required_alignment == 0,
                                 "atomic_ref: the object is not aligned to required_alignment");
    }
  }
  explicit atomic_ref(T&&) = delete;
  constexpr atomic_ref(const atomic_ref&) noexcept = default;
  template <class U>
    requires is_same_v<remove_cv_t<U>, remove_cv_t<T>> && is_convertible_v<U*, T*>
  constexpr atomic_ref(const atomic_ref<U>& ref) noexcept
      : ycxx::adl_free::atomic_ref_base<T>(static_cast<T*>(ref.address())) {}
  atomic_ref& operator=(const atomic_ref&) = delete;
  using ycxx::adl_free::atomic_ref_base<T>::operator=;

  constexpr value_type fetch_add(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::add, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_sub(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::sub, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_and(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::and_, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_or(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::or_, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_xor(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::xor_, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_max(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_max<value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_min(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_min<value_type>(this->ptr_, a, o);
  }
  constexpr void store_add(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::add, value_type>(this->ptr_, a, o);
  }
  constexpr void store_sub(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::sub, value_type>(this->ptr_, a, o);
  }
  constexpr void store_and(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::and_, value_type>(this->ptr_, a, o);
  }
  constexpr void store_or(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::or_, value_type>(this->ptr_, a, o);
  }
  constexpr void store_xor(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::xor_, value_type>(this->ptr_, a, o);
  }
  constexpr void store_max(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_max<value_type>(this->ptr_, a, o);
  }
  constexpr void store_min(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_min<value_type>(this->ptr_, a, o);
  }
  constexpr value_type operator++(int) const noexcept
    requires(!is_const_v<T>)
  {
    return fetch_add(1);
  }
  constexpr value_type operator--(int) const noexcept
    requires(!is_const_v<T>)
  {
    return fetch_sub(1);
  }
  constexpr value_type operator++() const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_wrap_add<value_type>(fetch_add(1), 1);
  }
  constexpr value_type operator--() const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_wrap_add<value_type>(fetch_sub(1), -1);
  }
  constexpr value_type operator+=(value_type a) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_wrap_add<value_type>(fetch_add(a), a);
  }
  constexpr value_type operator-=(value_type a) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_wrap_sub<value_type>(fetch_sub(a), a);
  }
  constexpr value_type operator&=(value_type a) const noexcept
    requires(!is_const_v<T>)
  {
    return static_cast<value_type>(fetch_and(a) & a);
  }
  constexpr value_type operator|=(value_type a) const noexcept
    requires(!is_const_v<T>)
  {
    return static_cast<value_type>(fetch_or(a) | a);
  }
  constexpr value_type operator^=(value_type a) const noexcept
    requires(!is_const_v<T>)
  {
    return static_cast<value_type>(fetch_xor(a) ^ a);
  }
};

// [atomics.ref.float]
template <class T>
  requires ycxx::detail::atomic_floating<remove_cv_t<T>>
struct atomic_ref<T> : ycxx::adl_free::atomic_ref_base<T> {
  using value_type = remove_cv_t<T>;
  using difference_type = value_type;

  constexpr explicit atomic_ref(T& obj) : ycxx::adl_free::atomic_ref_base<T>(__builtin_addressof(obj)) {
    if !consteval {
      ycxx::detail::precondition(reinterpret_cast<__UINTPTR_TYPE__>(this->ptr_) % this->required_alignment == 0,
                                 "atomic_ref: the object is not aligned to required_alignment");
    }
  }
  explicit atomic_ref(T&&) = delete;
  constexpr atomic_ref(const atomic_ref&) noexcept = default;
  template <class U>
    requires is_same_v<remove_cv_t<U>, remove_cv_t<T>> && is_convertible_v<U*, T*>
  constexpr atomic_ref(const atomic_ref<U>& ref) noexcept
      : ycxx::adl_free::atomic_ref_base<T>(static_cast<T*>(ref.address())) {}
  atomic_ref& operator=(const atomic_ref&) = delete;
  using ycxx::adl_free::atomic_ref_base<T>::operator=;

  constexpr value_type fetch_add(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::add, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_sub(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::sub, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_max(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_min(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_fmaximum(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_fminimum(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_fmaximum_num(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_fminimum_num(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, value_type>(this->ptr_, a, o);
  }
  constexpr void store_add(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::add, value_type>(this->ptr_, a, o);
  }
  constexpr void store_sub(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::sub, value_type>(this->ptr_, a, o);
  }
  constexpr void store_max(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, value_type>(this->ptr_, a, o);
  }
  constexpr void store_min(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, value_type>(this->ptr_, a, o);
  }
  constexpr void store_fmaximum(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum, value_type>(this->ptr_, a, o);
  }
  constexpr void store_fminimum(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum, value_type>(this->ptr_, a, o);
  }
  constexpr void store_fmaximum_num(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, value_type>(this->ptr_, a, o);
  }
  constexpr void store_fminimum_num(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, value_type>(this->ptr_, a, o);
  }
  constexpr value_type operator+=(value_type a) const noexcept
    requires(!is_const_v<T>)
  {
    return static_cast<value_type>(fetch_add(a) + a);
  }
  constexpr value_type operator-=(value_type a) const noexcept
    requires(!is_const_v<T>)
  {
    return static_cast<value_type>(fetch_sub(a) - a);
  }
};

// [atomics.ref.pointer]
template <class T>
  requires is_pointer_v<remove_cv_t<T>> && is_object_v<remove_pointer_t<remove_cv_t<T>>>
struct atomic_ref<T> : ycxx::adl_free::atomic_ref_base<T> {
  using value_type = remove_cv_t<T>;
  using difference_type = ptrdiff_t;

  constexpr explicit atomic_ref(T& obj) : ycxx::adl_free::atomic_ref_base<T>(__builtin_addressof(obj)) {
    if !consteval {
      ycxx::detail::precondition(reinterpret_cast<__UINTPTR_TYPE__>(this->ptr_) % this->required_alignment == 0,
                                 "atomic_ref: the object is not aligned to required_alignment");
    }
  }
  explicit atomic_ref(T&&) = delete;
  constexpr atomic_ref(const atomic_ref&) noexcept = default;
  template <class U>
    requires is_same_v<remove_cv_t<U>, remove_cv_t<T>> && is_convertible_v<U*, T*>
  constexpr atomic_ref(const atomic_ref<U>& ref) noexcept
      : ycxx::adl_free::atomic_ref_base<T>(static_cast<T*>(ref.address())) {}
  atomic_ref& operator=(const atomic_ref&) = delete;
  using ycxx::adl_free::atomic_ref_base<T>::operator=;

  constexpr value_type fetch_add(ptrdiff_t a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_ptr<value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_sub(ptrdiff_t a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_ptr<value_type>(this->ptr_, -a, o);
  }
  constexpr value_type fetch_max(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_max<value_type>(this->ptr_, a, o);
  }
  constexpr value_type fetch_min(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_fetch_min<value_type>(this->ptr_, a, o);
  }
  constexpr void store_add(ptrdiff_t a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_ptr<value_type>(this->ptr_, a, o);
  }
  constexpr void store_sub(ptrdiff_t a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_ptr<value_type>(this->ptr_, -a, o);
  }
  constexpr void store_max(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_max<value_type>(this->ptr_, a, o);
  }
  constexpr void store_min(value_type a, memory_order o = memory_order::seq_cst) const noexcept
    requires(!is_const_v<T>)
  {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_min<value_type>(this->ptr_, a, o);
  }
  constexpr value_type operator++(int) const noexcept
    requires(!is_const_v<T>)
  {
    return fetch_add(1);
  }
  constexpr value_type operator--(int) const noexcept
    requires(!is_const_v<T>)
  {
    return fetch_sub(1);
  }
  constexpr value_type operator++() const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_ptr_add<value_type>(fetch_add(1), 1);
  }
  constexpr value_type operator--() const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_ptr_add<value_type>(fetch_sub(1), -1);
  }
  constexpr value_type operator+=(difference_type a) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_ptr_add<value_type>(fetch_add(a), a);
  }
  constexpr value_type operator-=(difference_type a) const noexcept
    requires(!is_const_v<T>)
  {
    return ::ycxx::detail::atomic_ptr_add<value_type>(fetch_sub(a), -a);
  }
};

// ---- [atomics.types.generic] -------------------------------------------------------------------
template <class T>
struct atomic : ycxx::adl_free::atomic_base<T> {
  static_assert(is_trivially_copyable_v<T> && is_copy_constructible_v<T> && is_move_constructible_v<T> &&
                    is_copy_assignable_v<T> && is_move_assignable_v<T>,
                "atomic<T> needs a trivially copyable, copy and move constructible and assignable T");
  static_assert(is_same_v<T, remove_cv_t<T>>, "atomic<T> needs a cv-unqualified T");

  using value_type = T;

  constexpr atomic() noexcept(is_nothrow_default_constructible_v<T>)
    requires is_default_constructible_v<T>
      : ycxx::adl_free::atomic_base<T>() {}
  constexpr atomic(T desired) noexcept : ycxx::adl_free::atomic_base<T>(desired) {}
  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;
  atomic& operator=(const atomic&) volatile = delete;
  using ycxx::adl_free::atomic_base<T>::operator=;
};

// [atomics.types.int]
template <class T>
  requires ycxx::detail::atomic_integral<T> && is_same_v<T, remove_cv_t<T>>
struct atomic<T> : ycxx::adl_free::atomic_base<T> {
  using value_type = T;
  using difference_type = value_type;

  constexpr atomic() noexcept : ycxx::adl_free::atomic_base<T>() {}
  constexpr atomic(T desired) noexcept : ycxx::adl_free::atomic_base<T>(desired) {}
  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;
  atomic& operator=(const atomic&) volatile = delete;
  using ycxx::adl_free::atomic_base<T>::operator=;

  T fetch_add(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::add, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_add(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::add, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_sub(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::sub, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_sub(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::sub, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_and(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::and_, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_and(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::and_, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_or(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::or_, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_or(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::or_, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_xor(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::xor_, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_xor(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::xor_, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_max(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_max<T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_max(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_max<T>(__builtin_addressof(this->v_), a, o); }
  T fetch_min(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_min<T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_min(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_min<T>(__builtin_addressof(this->v_), a, o); }
  void store_add(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::add, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_add(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::add, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_sub(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::sub, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_sub(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::sub, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_and(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::and_, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_and(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::and_, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_or(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::or_, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_or(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::or_, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_xor(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::xor_, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_xor(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_int<::ycxx::detail::atomic_int_op::xor_, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_max(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_max<T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_max(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_max<T>(__builtin_addressof(this->v_), a, o);
  }
  void store_min(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_min<T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_min(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_min<T>(__builtin_addressof(this->v_), a, o);
  }
  T operator++(int) volatile noexcept { return fetch_add(1); }
  T operator--(int) volatile noexcept { return fetch_sub(1); }
  T operator++() volatile noexcept { return ::ycxx::detail::atomic_wrap_add<T>(fetch_add(1), 1); }
  T operator--() volatile noexcept { return ::ycxx::detail::atomic_wrap_add<T>(fetch_sub(1), -1); }
  constexpr T operator++(int) noexcept { return fetch_add(1); }
  constexpr T operator--(int) noexcept { return fetch_sub(1); }
  constexpr T operator++() noexcept { return ::ycxx::detail::atomic_wrap_add<T>(fetch_add(1), 1); }
  constexpr T operator--() noexcept { return ::ycxx::detail::atomic_wrap_add<T>(fetch_sub(1), -1); }
  T operator+=(T a) volatile noexcept { return ::ycxx::detail::atomic_wrap_add<T>(fetch_add(a), a); }
  constexpr T operator+=(T a) noexcept { return ::ycxx::detail::atomic_wrap_add<T>(fetch_add(a), a); }
  T operator-=(T a) volatile noexcept { return ::ycxx::detail::atomic_wrap_sub<T>(fetch_sub(a), a); }
  constexpr T operator-=(T a) noexcept { return ::ycxx::detail::atomic_wrap_sub<T>(fetch_sub(a), a); }
  T operator&=(T a) volatile noexcept { return static_cast<T>(fetch_and(a) & a); }
  constexpr T operator&=(T a) noexcept { return static_cast<T>(fetch_and(a) & a); }
  T operator|=(T a) volatile noexcept { return static_cast<T>(fetch_or(a) | a); }
  constexpr T operator|=(T a) noexcept { return static_cast<T>(fetch_or(a) | a); }
  T operator^=(T a) volatile noexcept { return static_cast<T>(fetch_xor(a) ^ a); }
  constexpr T operator^=(T a) noexcept { return static_cast<T>(fetch_xor(a) ^ a); }
};

// [atomics.types.float]
template <class T>
  requires ycxx::detail::atomic_floating<T> && is_same_v<T, remove_cv_t<T>>
struct atomic<T> : ycxx::adl_free::atomic_base<T> {
  using value_type = T;
  using difference_type = value_type;

  constexpr atomic() noexcept : ycxx::adl_free::atomic_base<T>() {}
  constexpr atomic(T desired) noexcept : ycxx::adl_free::atomic_base<T>(desired) {}
  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;
  atomic& operator=(const atomic&) volatile = delete;
  using ycxx::adl_free::atomic_base<T>::operator=;

  T fetch_add(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::add, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_add(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::add, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_sub(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::sub, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_sub(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::sub, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_max(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_max(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_min(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_min(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_fmaximum(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_fmaximum(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_fminimum(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_fminimum(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_fmaximum_num(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_fmaximum_num(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, T>(__builtin_addressof(this->v_), a, o); }
  T fetch_fminimum_num(T a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, T>(__builtin_addressof(this->v_), a, o); }
  constexpr T fetch_fminimum_num(T a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, T>(__builtin_addressof(this->v_), a, o); }
  void store_add(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::add, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_add(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::add, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_sub(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::sub, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_sub(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::sub, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_max(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_max(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_min(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_min(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_fmaximum(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_fmaximum(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_fminimum(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_fminimum(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_fmaximum_num(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_fmaximum_num(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::maximum_num, T>(__builtin_addressof(this->v_), a, o);
  }
  void store_fminimum_num(T a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, T>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_fminimum_num(T a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_fp<::ycxx::detail::atomic_fp_op::minimum_num, T>(__builtin_addressof(this->v_), a, o);
  }
  T operator+=(T a) volatile noexcept { return static_cast<T>(fetch_add(a) + a); }
  constexpr T operator+=(T a) noexcept { return static_cast<T>(fetch_add(a) + a); }
  T operator-=(T a) volatile noexcept { return static_cast<T>(fetch_sub(a) - a); }
  constexpr T operator-=(T a) noexcept { return static_cast<T>(fetch_sub(a) - a); }
};

// [atomics.types.pointer]
template <class T>
struct atomic<T*> : ycxx::adl_free::atomic_base<T*> {
  using value_type = T*;
  using difference_type = ptrdiff_t;

  constexpr atomic() noexcept : ycxx::adl_free::atomic_base<T*>() {}
  constexpr atomic(T* desired) noexcept : ycxx::adl_free::atomic_base<T*>(desired) {}
  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;
  atomic& operator=(const atomic&) volatile = delete;
  using ycxx::adl_free::atomic_base<T*>::operator=;

  T* fetch_add(ptrdiff_t a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_ptr<T*>(__builtin_addressof(this->v_), a, o); }
  constexpr T* fetch_add(ptrdiff_t a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_ptr<T*>(__builtin_addressof(this->v_), a, o); }
  T* fetch_sub(ptrdiff_t a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_ptr<T*>(__builtin_addressof(this->v_), -a, o); }
  constexpr T* fetch_sub(ptrdiff_t a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_ptr<T*>(__builtin_addressof(this->v_), -a, o); }
  T* fetch_max(T* a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_max<T*>(__builtin_addressof(this->v_), a, o); }
  constexpr T* fetch_max(T* a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_max<T*>(__builtin_addressof(this->v_), a, o); }
  T* fetch_min(T* a, memory_order o = memory_order::seq_cst) volatile noexcept { return ::ycxx::detail::atomic_fetch_min<T*>(__builtin_addressof(this->v_), a, o); }
  constexpr T* fetch_min(T* a, memory_order o = memory_order::seq_cst) noexcept { return ::ycxx::detail::atomic_fetch_min<T*>(__builtin_addressof(this->v_), a, o); }
  void store_add(ptrdiff_t a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_ptr<T*>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_add(ptrdiff_t a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_ptr<T*>(__builtin_addressof(this->v_), a, o);
  }
  void store_sub(ptrdiff_t a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_ptr<T*>(__builtin_addressof(this->v_), -a, o);
  }
  constexpr void store_sub(ptrdiff_t a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_ptr<T*>(__builtin_addressof(this->v_), -a, o);
  }
  void store_max(T* a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_max<T*>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_max(T* a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_max<T*>(__builtin_addressof(this->v_), a, o);
  }
  void store_min(T* a, memory_order o = memory_order::seq_cst) volatile noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_min<T*>(__builtin_addressof(this->v_), a, o);
  }
  constexpr void store_min(T* a, memory_order o = memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    (void)::ycxx::detail::atomic_fetch_min<T*>(__builtin_addressof(this->v_), a, o);
  }
  T* operator++(int) volatile noexcept { return fetch_add(1); }
  T* operator--(int) volatile noexcept { return fetch_sub(1); }
  T* operator++() volatile noexcept { return ::ycxx::detail::atomic_ptr_add<T*>(fetch_add(1), 1); }
  T* operator--() volatile noexcept { return ::ycxx::detail::atomic_ptr_add<T*>(fetch_sub(1), -1); }
  constexpr T* operator++(int) noexcept { return fetch_add(1); }
  constexpr T* operator--(int) noexcept { return fetch_sub(1); }
  constexpr T* operator++() noexcept { return ::ycxx::detail::atomic_ptr_add<T*>(fetch_add(1), 1); }
  constexpr T* operator--() noexcept { return ::ycxx::detail::atomic_ptr_add<T*>(fetch_sub(1), -1); }
  T* operator+=(ptrdiff_t a) volatile noexcept { return ::ycxx::detail::atomic_ptr_add<T*>(fetch_add(a), a); }
  constexpr T* operator+=(ptrdiff_t a) noexcept { return ::ycxx::detail::atomic_ptr_add<T*>(fetch_add(a), a); }
  T* operator-=(ptrdiff_t a) volatile noexcept { return ::ycxx::detail::atomic_ptr_add<T*>(fetch_sub(a), -a); }
  constexpr T* operator-=(ptrdiff_t a) noexcept { return ::ycxx::detail::atomic_ptr_add<T*>(fetch_sub(a), -a); }
};

// ---- [atomics.flag] ----------------------------------------------------------------------------
struct atomic_flag {
private:
  unsigned char v_;

public:
  constexpr atomic_flag() noexcept : v_(0) {}
  atomic_flag(const atomic_flag&) = delete;
  atomic_flag& operator=(const atomic_flag&) = delete;
  atomic_flag& operator=(const atomic_flag&) volatile = delete;

  bool test(memory_order o = memory_order::seq_cst) const volatile noexcept {
    return ycxx::detail::atomic_load<unsigned char>(&v_, o) != 0;
  }
  constexpr bool test(memory_order o = memory_order::seq_cst) const noexcept {
    return ycxx::detail::atomic_load<unsigned char>(&v_, o) != 0;
  }
  bool test_and_set(memory_order o = memory_order::seq_cst) volatile noexcept {
    return ycxx::detail::atomic_exchange<unsigned char>(&v_, 1, o) != 0;
  }
  constexpr bool test_and_set(memory_order o = memory_order::seq_cst) noexcept {
    return ycxx::detail::atomic_exchange<unsigned char>(&v_, 1, o) != 0;
  }
  void clear(memory_order o = memory_order::seq_cst) volatile noexcept {
    ycxx::detail::atomic_store<unsigned char>(&v_, 0, o);
  }
  constexpr void clear(memory_order o = memory_order::seq_cst) noexcept {
    ycxx::detail::atomic_store<unsigned char>(&v_, 0, o);
  }
  void wait(bool old, memory_order o = memory_order::seq_cst) const volatile noexcept {
    ycxx::detail::atomic_wait<unsigned char>(&v_, old ? 1 : 0, o);
  }
  constexpr void wait(bool old, memory_order o = memory_order::seq_cst) const noexcept {
    ycxx::detail::atomic_wait<unsigned char>(&v_, old ? 1 : 0, o);
  }
  void notify_one() volatile noexcept { ycxx::detail::atomic_notify_all(&v_); }
  constexpr void notify_one() noexcept { ycxx::detail::atomic_notify_all(&v_); }
  void notify_all() volatile noexcept { ycxx::detail::atomic_notify_all(&v_); }
  constexpr void notify_all() noexcept { ycxx::detail::atomic_notify_all(&v_); }
};

inline bool atomic_flag_test(const volatile atomic_flag* object) noexcept { return object->test(); }
constexpr bool atomic_flag_test(const atomic_flag* object) noexcept { return object->test(); }
inline bool atomic_flag_test_explicit(const volatile atomic_flag* object, memory_order o) noexcept {
  return object->test(o);
}
constexpr bool atomic_flag_test_explicit(const atomic_flag* object, memory_order o) noexcept { return object->test(o); }
inline bool atomic_flag_test_and_set(volatile atomic_flag* object) noexcept { return object->test_and_set(); }
constexpr bool atomic_flag_test_and_set(atomic_flag* object) noexcept { return object->test_and_set(); }
inline bool atomic_flag_test_and_set_explicit(volatile atomic_flag* object, memory_order o) noexcept {
  return object->test_and_set(o);
}
constexpr bool atomic_flag_test_and_set_explicit(atomic_flag* object, memory_order o) noexcept {
  return object->test_and_set(o);
}
inline void atomic_flag_clear(volatile atomic_flag* object) noexcept { object->clear(); }
constexpr void atomic_flag_clear(atomic_flag* object) noexcept { object->clear(); }
inline void atomic_flag_clear_explicit(volatile atomic_flag* object, memory_order o) noexcept { object->clear(o); }
constexpr void atomic_flag_clear_explicit(atomic_flag* object, memory_order o) noexcept { object->clear(o); }
inline void atomic_flag_wait(const volatile atomic_flag* object, bool old) noexcept { object->wait(old); }
constexpr void atomic_flag_wait(const atomic_flag* object, bool old) noexcept { object->wait(old); }
inline void atomic_flag_wait_explicit(const volatile atomic_flag* object, bool old, memory_order o) noexcept {
  object->wait(old, o);
}
constexpr void atomic_flag_wait_explicit(const atomic_flag* object, bool old, memory_order o) noexcept {
  object->wait(old, o);
}
inline void atomic_flag_notify_one(volatile atomic_flag* object) noexcept { object->notify_one(); }
constexpr void atomic_flag_notify_one(atomic_flag* object) noexcept { object->notify_one(); }
inline void atomic_flag_notify_all(volatile atomic_flag* object) noexcept { object->notify_all(); }
constexpr void atomic_flag_notify_all(atomic_flag* object) noexcept { object->notify_all(); }

// ---- [atomics.fences] --------------------------------------------------------------------------
extern "C" constexpr void atomic_thread_fence(memory_order order) noexcept {
  if !consteval {
    __atomic_thread_fence(static_cast<int>(order));
  }
}
extern "C" constexpr void atomic_signal_fence(memory_order order) noexcept {
  if !consteval {
    __atomic_signal_fence(static_cast<int>(order));
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
template <class T>
bool atomic_is_lock_free(const volatile atomic<T>* object) noexcept {
  return object->is_lock_free();
}
template <class T>
bool atomic_is_lock_free(const atomic<T>* object) noexcept {
  return object->is_lock_free();
}
template <class T>
void atomic_store(volatile atomic<T>* object, typename atomic<T>::value_type desired) noexcept {
  object->store(desired);
}
template <class T>
constexpr void atomic_store(atomic<T>* object, typename atomic<T>::value_type desired) noexcept {
  object->store(desired);
}
template <class T>
void atomic_store_explicit(volatile atomic<T>* object, typename atomic<T>::value_type desired, memory_order o) noexcept {
  object->store(desired, o);
}
template <class T>
constexpr void atomic_store_explicit(atomic<T>* object, typename atomic<T>::value_type desired, memory_order o) noexcept {
  object->store(desired, o);
}
template <class T>
T atomic_load(const volatile atomic<T>* object) noexcept {
  return object->load();
}
template <class T>
constexpr T atomic_load(const atomic<T>* object) noexcept {
  return object->load();
}
template <class T>
T atomic_load_explicit(const volatile atomic<T>* object, memory_order o) noexcept {
  return object->load(o);
}
template <class T>
constexpr T atomic_load_explicit(const atomic<T>* object, memory_order o) noexcept {
  return object->load(o);
}
template <class T>
T atomic_exchange(volatile atomic<T>* object, typename atomic<T>::value_type desired) noexcept {
  return object->exchange(desired);
}
template <class T>
constexpr T atomic_exchange(atomic<T>* object, typename atomic<T>::value_type desired) noexcept {
  return object->exchange(desired);
}
template <class T>
T atomic_exchange_explicit(volatile atomic<T>* object, typename atomic<T>::value_type desired, memory_order o) noexcept {
  return object->exchange(desired, o);
}
template <class T>
constexpr T atomic_exchange_explicit(atomic<T>* object, typename atomic<T>::value_type desired, memory_order o) noexcept {
  return object->exchange(desired, o);
}
template <class T>
bool atomic_compare_exchange_weak(volatile atomic<T>* object, typename atomic<T>::value_type* expected,
                                  typename atomic<T>::value_type desired) noexcept {
  return object->compare_exchange_weak(*expected, desired);
}
template <class T>
constexpr bool atomic_compare_exchange_weak(atomic<T>* object, typename atomic<T>::value_type* expected,
                                            typename atomic<T>::value_type desired) noexcept {
  return object->compare_exchange_weak(*expected, desired);
}
template <class T>
bool atomic_compare_exchange_strong(volatile atomic<T>* object, typename atomic<T>::value_type* expected,
                                    typename atomic<T>::value_type desired) noexcept {
  return object->compare_exchange_strong(*expected, desired);
}
template <class T>
constexpr bool atomic_compare_exchange_strong(atomic<T>* object, typename atomic<T>::value_type* expected,
                                              typename atomic<T>::value_type desired) noexcept {
  return object->compare_exchange_strong(*expected, desired);
}
template <class T>
bool atomic_compare_exchange_weak_explicit(volatile atomic<T>* object, typename atomic<T>::value_type* expected,
                                           typename atomic<T>::value_type desired, memory_order s,
                                           memory_order f) noexcept {
  return object->compare_exchange_weak(*expected, desired, s, f);
}
template <class T>
constexpr bool atomic_compare_exchange_weak_explicit(atomic<T>* object, typename atomic<T>::value_type* expected,
                                                     typename atomic<T>::value_type desired, memory_order s,
                                                     memory_order f) noexcept {
  return object->compare_exchange_weak(*expected, desired, s, f);
}
template <class T>
bool atomic_compare_exchange_strong_explicit(volatile atomic<T>* object, typename atomic<T>::value_type* expected,
                                             typename atomic<T>::value_type desired, memory_order s,
                                             memory_order f) noexcept {
  return object->compare_exchange_strong(*expected, desired, s, f);
}
template <class T>
constexpr bool atomic_compare_exchange_strong_explicit(atomic<T>* object, typename atomic<T>::value_type* expected,
                                                       typename atomic<T>::value_type desired, memory_order s,
                                                       memory_order f) noexcept {
  return object->compare_exchange_strong(*expected, desired, s, f);
}
template <class T>
T atomic_fetch_add(volatile atomic<T>* object, typename atomic<T>::difference_type operand) noexcept {
  return object->fetch_add(operand);
}
template <class T>
constexpr T atomic_fetch_add(atomic<T>* object, typename atomic<T>::difference_type operand) noexcept {
  return object->fetch_add(operand);
}
template <class T>
T atomic_fetch_add_explicit(volatile atomic<T>* object, typename atomic<T>::difference_type operand, memory_order o) noexcept {
  return object->fetch_add(operand, o);
}
template <class T>
constexpr T atomic_fetch_add_explicit(atomic<T>* object, typename atomic<T>::difference_type operand, memory_order o) noexcept {
  return object->fetch_add(operand, o);
}
template <class T>
T atomic_fetch_sub(volatile atomic<T>* object, typename atomic<T>::difference_type operand) noexcept {
  return object->fetch_sub(operand);
}
template <class T>
constexpr T atomic_fetch_sub(atomic<T>* object, typename atomic<T>::difference_type operand) noexcept {
  return object->fetch_sub(operand);
}
template <class T>
T atomic_fetch_sub_explicit(volatile atomic<T>* object, typename atomic<T>::difference_type operand, memory_order o) noexcept {
  return object->fetch_sub(operand, o);
}
template <class T>
constexpr T atomic_fetch_sub_explicit(atomic<T>* object, typename atomic<T>::difference_type operand, memory_order o) noexcept {
  return object->fetch_sub(operand, o);
}
template <class T>
T atomic_fetch_and(volatile atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  return object->fetch_and(operand);
}
template <class T>
constexpr T atomic_fetch_and(atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  return object->fetch_and(operand);
}
template <class T>
T atomic_fetch_and_explicit(volatile atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  return object->fetch_and(operand, o);
}
template <class T>
constexpr T atomic_fetch_and_explicit(atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  return object->fetch_and(operand, o);
}
template <class T>
T atomic_fetch_or(volatile atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  return object->fetch_or(operand);
}
template <class T>
constexpr T atomic_fetch_or(atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  return object->fetch_or(operand);
}
template <class T>
T atomic_fetch_or_explicit(volatile atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  return object->fetch_or(operand, o);
}
template <class T>
constexpr T atomic_fetch_or_explicit(atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  return object->fetch_or(operand, o);
}
template <class T>
T atomic_fetch_xor(volatile atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  return object->fetch_xor(operand);
}
template <class T>
constexpr T atomic_fetch_xor(atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  return object->fetch_xor(operand);
}
template <class T>
T atomic_fetch_xor_explicit(volatile atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  return object->fetch_xor(operand, o);
}
template <class T>
constexpr T atomic_fetch_xor_explicit(atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  return object->fetch_xor(operand, o);
}
template <class T>
T atomic_fetch_max(volatile atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  return object->fetch_max(operand);
}
template <class T>
constexpr T atomic_fetch_max(atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  return object->fetch_max(operand);
}
template <class T>
T atomic_fetch_max_explicit(volatile atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  return object->fetch_max(operand, o);
}
template <class T>
constexpr T atomic_fetch_max_explicit(atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  return object->fetch_max(operand, o);
}
template <class T>
T atomic_fetch_min(volatile atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  return object->fetch_min(operand);
}
template <class T>
constexpr T atomic_fetch_min(atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  return object->fetch_min(operand);
}
template <class T>
T atomic_fetch_min_explicit(volatile atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  return object->fetch_min(operand, o);
}
template <class T>
constexpr T atomic_fetch_min_explicit(atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  return object->fetch_min(operand, o);
}
template <class T>
void atomic_store_add(volatile atomic<T>* object, typename atomic<T>::difference_type operand) noexcept {
  object->store_add(operand);
}
template <class T>
constexpr void atomic_store_add(atomic<T>* object, typename atomic<T>::difference_type operand) noexcept {
  object->store_add(operand);
}
template <class T>
void atomic_store_add_explicit(volatile atomic<T>* object, typename atomic<T>::difference_type operand, memory_order o) noexcept {
  object->store_add(operand, o);
}
template <class T>
constexpr void atomic_store_add_explicit(atomic<T>* object, typename atomic<T>::difference_type operand, memory_order o) noexcept {
  object->store_add(operand, o);
}
template <class T>
void atomic_store_sub(volatile atomic<T>* object, typename atomic<T>::difference_type operand) noexcept {
  object->store_sub(operand);
}
template <class T>
constexpr void atomic_store_sub(atomic<T>* object, typename atomic<T>::difference_type operand) noexcept {
  object->store_sub(operand);
}
template <class T>
void atomic_store_sub_explicit(volatile atomic<T>* object, typename atomic<T>::difference_type operand, memory_order o) noexcept {
  object->store_sub(operand, o);
}
template <class T>
constexpr void atomic_store_sub_explicit(atomic<T>* object, typename atomic<T>::difference_type operand, memory_order o) noexcept {
  object->store_sub(operand, o);
}
template <class T>
void atomic_store_and(volatile atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  object->store_and(operand);
}
template <class T>
constexpr void atomic_store_and(atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  object->store_and(operand);
}
template <class T>
void atomic_store_and_explicit(volatile atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  object->store_and(operand, o);
}
template <class T>
constexpr void atomic_store_and_explicit(atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  object->store_and(operand, o);
}
template <class T>
void atomic_store_or(volatile atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  object->store_or(operand);
}
template <class T>
constexpr void atomic_store_or(atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  object->store_or(operand);
}
template <class T>
void atomic_store_or_explicit(volatile atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  object->store_or(operand, o);
}
template <class T>
constexpr void atomic_store_or_explicit(atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  object->store_or(operand, o);
}
template <class T>
void atomic_store_xor(volatile atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  object->store_xor(operand);
}
template <class T>
constexpr void atomic_store_xor(atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  object->store_xor(operand);
}
template <class T>
void atomic_store_xor_explicit(volatile atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  object->store_xor(operand, o);
}
template <class T>
constexpr void atomic_store_xor_explicit(atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  object->store_xor(operand, o);
}
template <class T>
void atomic_store_max(volatile atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  object->store_max(operand);
}
template <class T>
constexpr void atomic_store_max(atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  object->store_max(operand);
}
template <class T>
void atomic_store_max_explicit(volatile atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  object->store_max(operand, o);
}
template <class T>
constexpr void atomic_store_max_explicit(atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  object->store_max(operand, o);
}
template <class T>
void atomic_store_min(volatile atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  object->store_min(operand);
}
template <class T>
constexpr void atomic_store_min(atomic<T>* object, typename atomic<T>::value_type operand) noexcept {
  object->store_min(operand);
}
template <class T>
void atomic_store_min_explicit(volatile atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  object->store_min(operand, o);
}
template <class T>
constexpr void atomic_store_min_explicit(atomic<T>* object, typename atomic<T>::value_type operand, memory_order o) noexcept {
  object->store_min(operand, o);
}
template <class T>
void atomic_wait(const volatile atomic<T>* object, typename atomic<T>::value_type old) noexcept {
  object->wait(old);
}
template <class T>
constexpr void atomic_wait(const atomic<T>* object, typename atomic<T>::value_type old) noexcept {
  object->wait(old);
}
template <class T>
void atomic_wait_explicit(const volatile atomic<T>* object, typename atomic<T>::value_type old, memory_order o) noexcept {
  object->wait(old, o);
}
template <class T>
constexpr void atomic_wait_explicit(const atomic<T>* object, typename atomic<T>::value_type old, memory_order o) noexcept {
  object->wait(old, o);
}
template <class T>
void atomic_notify_one(volatile atomic<T>* object) noexcept {
  object->notify_one();
}
template <class T>
constexpr void atomic_notify_one(atomic<T>* object) noexcept {
  object->notify_one();
}
template <class T>
void atomic_notify_all(volatile atomic<T>* object) noexcept {
  object->notify_all();
}
template <class T>
constexpr void atomic_notify_all(atomic<T>* object) noexcept {
  object->notify_all();
}

// [depr.atomics.nonmembers]
template <class T>
[[deprecated]] void atomic_init(volatile atomic<T>* object, typename atomic<T>::value_type desired) noexcept {
  object->store(desired, memory_order::relaxed);
}
template <class T>
[[deprecated]] void atomic_init(atomic<T>* object, typename atomic<T>::value_type desired) noexcept {
  object->store(desired, memory_order::relaxed);
}

} // namespace std
