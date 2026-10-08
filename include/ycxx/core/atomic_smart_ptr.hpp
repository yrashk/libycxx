// libycxx core: atomic<shared_ptr<T>> and atomic<weak_ptr<T>> ([util.smartptr.atomic]).
//
// Lock-based: every operation holds the lock of the runtime's striped lock table that belongs to
// the atomic object's address while it reads or swaps the stored pointer and adjusts use counts
// ([util.smartptr.atomic.general]/2); a replaced value is released after the lock is dropped, so
// no destructor or deallocation runs under it. A seq_cst operation adds a seq_cst fence. Waiting
// compares under the lock and blocks on the atomic wait slot of the object.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/atomic.hpp>
#include <ycxx/core/shared_ptr.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// Holds the striped lock of an atomic smart pointer (no-op during constant evaluation).
class __sp_atomic_guard {
  const volatile void* __addr_;

public:
  constexpr __sp_atomic_guard(const volatile void* __addr, std::memory_order __o) noexcept : __addr_(__addr) {
    if !consteval {
      ::__ycxx::__detail::__atomic_lock(__addr_);
      if (__o == std::memory_order::seq_cst)
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
    }
  }
  __sp_atomic_guard(const __sp_atomic_guard&) = delete;
  __sp_atomic_guard& operator=(const __sp_atomic_guard&) = delete;
  constexpr ~__sp_atomic_guard() {
    if !consteval {
      ::__ycxx::__detail::__atomic_unlock(__addr_);
    }
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {

// The base of atomic<shared_ptr<T>> and atomic<weak_ptr<T>>; P is shared_ptr<T> or weak_ptr<T>.
template <class _Pp>
class __sp_atomic {
protected:
  _Pp __p_;

  // [util.smartptr.atomic.shared]/16: the same stored pointer, and shared ownership or both empty.
  static constexpr bool equivalent(const _Pp& a, const _Pp& b) noexcept {
    if constexpr (requires { a.get(); })
      return a.get() == b.get() && a.owner_equal(b);
    else
      return ::__ycxx::__detail::__sp_access::__stored(a) == ::__ycxx::__detail::__sp_access::__stored(b) && a.owner_equal(b);
  }

public:
  static constexpr bool is_always_lock_free = false;
  bool is_lock_free() const noexcept { return false; }

  constexpr __sp_atomic() noexcept = default;
  constexpr __sp_atomic(_Pp __desired) noexcept : __p_(static_cast<_Pp&&>(__desired)) {}
  __sp_atomic(const __sp_atomic&) = delete;
  void operator=(const __sp_atomic&) = delete;

  constexpr _Pp load(std::memory_order __o = std::memory_order::seq_cst) const noexcept {
    ::__ycxx::__detail::__atomic_check_load(__o);
    ::__ycxx::__detail::__sp_atomic_guard __g(this, __o);
    return __p_;
  }
  constexpr operator _Pp() const noexcept { return load(); }
  constexpr void store(_Pp __desired, std::memory_order __o = std::memory_order::seq_cst) noexcept {
    ::__ycxx::__detail::__atomic_check_store(__o);
    {
      ::__ycxx::__detail::__sp_atomic_guard __g(this, __o);
      __p_.swap(__desired);
    }
    // desired (the old value) is released here, outside the lock.
  }
  constexpr void operator=(_Pp __desired) noexcept { store(static_cast<_Pp&&>(__desired)); }
  constexpr _Pp exchange(_Pp __desired, std::memory_order __o = std::memory_order::seq_cst) noexcept {
    {
      ::__ycxx::__detail::__sp_atomic_guard __g(this, __o);
      __p_.swap(__desired);
    }
    return __desired;
  }
  constexpr bool compare_exchange_weak(_Pp& expected, _Pp __desired, std::memory_order s, std::memory_order __f) noexcept {
    return compare_exchange_strong(expected, static_cast<_Pp&&>(__desired), s, __f);
  }
  constexpr bool compare_exchange_strong(_Pp& expected, _Pp __desired, std::memory_order s, std::memory_order __f) noexcept {
    ::__ycxx::__detail::__precondition(__f != std::memory_order::release && __f != std::memory_order::acq_rel,
                                 "atomic compare_exchange: failure order must be relaxed, acquire or seq_cst");
    _Pp __old; // the value to release after the lock is dropped
    {
      ::__ycxx::__detail::__sp_atomic_guard __g(this, s == std::memory_order::seq_cst || __f == std::memory_order::seq_cst
                                  ? std::memory_order::seq_cst
                                  : s);
      if (equivalent(__p_, expected)) {
        __p_.swap(__desired);
        __old.swap(__desired);
        return true;
      }
      __old = __p_; // the use count update is part of the atomic operation
    }
    expected.swap(__old);
    return false;
  }
  constexpr bool compare_exchange_weak(_Pp& expected, _Pp __desired, std::memory_order __o = std::memory_order::seq_cst) noexcept {
    return compare_exchange_strong(expected, static_cast<_Pp&&>(__desired), __o, ::__ycxx::__detail::__atomic_failure_order(__o));
  }
  constexpr bool compare_exchange_strong(_Pp& expected, _Pp __desired,
                                         std::memory_order __o = std::memory_order::seq_cst) noexcept {
    return compare_exchange_strong(expected, static_cast<_Pp&&>(__desired), __o, ::__ycxx::__detail::__atomic_failure_order(__o));
  }

  constexpr void wait(_Pp __old, std::memory_order __o = std::memory_order::seq_cst) const noexcept {
    ::__ycxx::__detail::__atomic_check_load(__o);
    if !consteval {
      ::__ycxx::__detail::__atomic_wait_until_done(this, [&] {
        ::__ycxx::__detail::__sp_atomic_guard __g(this, __o);
        return !equivalent(__p_, __old);
      });
    }
  }
  constexpr void notify_one() noexcept { ::__ycxx::__detail::atomic_notify_all(this); }
  constexpr void notify_all() noexcept { ::__ycxx::__detail::atomic_notify_all(this); }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [util.smartptr.atomic.shared]
template <class _Tp>
struct atomic<shared_ptr<_Tp>> : __ycxx::__adl_free::__sp_atomic<shared_ptr<_Tp>> {
  using value_type = shared_ptr<_Tp>;

  constexpr atomic() noexcept = default;
  constexpr atomic(nullptr_t) noexcept : atomic() {}
  constexpr atomic(shared_ptr<_Tp> __desired) noexcept
      : __ycxx::__adl_free::__sp_atomic<shared_ptr<_Tp>>(static_cast<shared_ptr<_Tp>&&>(__desired)) {}
  atomic(const atomic&) = delete;
  void operator=(const atomic&) = delete;
  constexpr void operator=(shared_ptr<_Tp> __desired) noexcept { this->store(static_cast<shared_ptr<_Tp>&&>(__desired)); }
  constexpr void operator=(nullptr_t) noexcept { this->store(nullptr); }
};

// [util.smartptr.atomic.weak]
template <class _Tp>
struct atomic<weak_ptr<_Tp>> : __ycxx::__adl_free::__sp_atomic<weak_ptr<_Tp>> {
  using value_type = weak_ptr<_Tp>;

  constexpr atomic() noexcept = default;
  constexpr atomic(weak_ptr<_Tp> __desired) noexcept
      : __ycxx::__adl_free::__sp_atomic<weak_ptr<_Tp>>(static_cast<weak_ptr<_Tp>&&>(__desired)) {}
  atomic(const atomic&) = delete;
  void operator=(const atomic&) = delete;
  constexpr void operator=(weak_ptr<_Tp> __desired) noexcept { this->store(static_cast<weak_ptr<_Tp>&&>(__desired)); }
};

}} // namespace std
