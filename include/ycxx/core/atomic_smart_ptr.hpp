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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// Holds the striped lock of an atomic smart pointer (no-op during constant evaluation).
class sp_atomic_guard {
  const volatile void* addr_;

public:
  constexpr sp_atomic_guard(const volatile void* addr, std::memory_order o) noexcept : addr_(addr) {
    if !consteval {
      ::ycxx::detail::atomic_lock(addr_);
      if (o == std::memory_order::seq_cst)
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
    }
  }
  sp_atomic_guard(const sp_atomic_guard&) = delete;
  sp_atomic_guard& operator=(const sp_atomic_guard&) = delete;
  constexpr ~sp_atomic_guard() {
    if !consteval {
      ::ycxx::detail::atomic_unlock(addr_);
    }
  }
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// The base of atomic<shared_ptr<T>> and atomic<weak_ptr<T>>; P is shared_ptr<T> or weak_ptr<T>.
template <class P>
class sp_atomic {
protected:
  P p_;

  // [util.smartptr.atomic.shared]/16: the same stored pointer, and shared ownership or both empty.
  static constexpr bool equivalent(const P& a, const P& b) noexcept {
    if constexpr (requires { a.get(); })
      return a.get() == b.get() && a.owner_equal(b);
    else
      return ::ycxx::detail::sp_access::stored(a) == ::ycxx::detail::sp_access::stored(b) && a.owner_equal(b);
  }

public:
  static constexpr bool is_always_lock_free = false;
  bool is_lock_free() const noexcept { return false; }

  constexpr sp_atomic() noexcept = default;
  constexpr sp_atomic(P desired) noexcept : p_(static_cast<P&&>(desired)) {}
  sp_atomic(const sp_atomic&) = delete;
  void operator=(const sp_atomic&) = delete;

  constexpr P load(std::memory_order o = std::memory_order::seq_cst) const noexcept {
    ::ycxx::detail::atomic_check_load(o);
    ::ycxx::detail::sp_atomic_guard g(this, o);
    return p_;
  }
  constexpr operator P() const noexcept { return load(); }
  constexpr void store(P desired, std::memory_order o = std::memory_order::seq_cst) noexcept {
    ::ycxx::detail::atomic_check_store(o);
    {
      ::ycxx::detail::sp_atomic_guard g(this, o);
      p_.swap(desired);
    }
    // desired (the old value) is released here, outside the lock.
  }
  constexpr void operator=(P desired) noexcept { store(static_cast<P&&>(desired)); }
  constexpr P exchange(P desired, std::memory_order o = std::memory_order::seq_cst) noexcept {
    {
      ::ycxx::detail::sp_atomic_guard g(this, o);
      p_.swap(desired);
    }
    return desired;
  }
  constexpr bool compare_exchange_weak(P& expected, P desired, std::memory_order s, std::memory_order f) noexcept {
    return compare_exchange_strong(expected, static_cast<P&&>(desired), s, f);
  }
  constexpr bool compare_exchange_strong(P& expected, P desired, std::memory_order s, std::memory_order f) noexcept {
    ::ycxx::detail::precondition(f != std::memory_order::release && f != std::memory_order::acq_rel,
                                 "atomic compare_exchange: failure order must be relaxed, acquire or seq_cst");
    P old; // the value to release after the lock is dropped
    {
      ::ycxx::detail::sp_atomic_guard g(this, s == std::memory_order::seq_cst || f == std::memory_order::seq_cst
                                  ? std::memory_order::seq_cst
                                  : s);
      if (equivalent(p_, expected)) {
        p_.swap(desired);
        old.swap(desired);
        return true;
      }
      old = p_; // the use count update is part of the atomic operation
    }
    expected.swap(old);
    return false;
  }
  constexpr bool compare_exchange_weak(P& expected, P desired, std::memory_order o = std::memory_order::seq_cst) noexcept {
    return compare_exchange_strong(expected, static_cast<P&&>(desired), o, ::ycxx::detail::atomic_failure_order(o));
  }
  constexpr bool compare_exchange_strong(P& expected, P desired,
                                         std::memory_order o = std::memory_order::seq_cst) noexcept {
    return compare_exchange_strong(expected, static_cast<P&&>(desired), o, ::ycxx::detail::atomic_failure_order(o));
  }

  constexpr void wait(P old, std::memory_order o = std::memory_order::seq_cst) const noexcept {
    ::ycxx::detail::atomic_check_load(o);
    if !consteval {
      ::ycxx::detail::atomic_wait_until_done(this, [&] {
        ::ycxx::detail::sp_atomic_guard g(this, o);
        return !equivalent(p_, old);
      });
    }
  }
  constexpr void notify_one() noexcept { ::ycxx::detail::atomic_notify_all(this); }
  constexpr void notify_all() noexcept { ::ycxx::detail::atomic_notify_all(this); }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

// [util.smartptr.atomic.shared]
template <class T>
struct atomic<shared_ptr<T>> : ycxx::adl_free::sp_atomic<shared_ptr<T>> {
  using value_type = shared_ptr<T>;

  constexpr atomic() noexcept = default;
  constexpr atomic(nullptr_t) noexcept : atomic() {}
  constexpr atomic(shared_ptr<T> desired) noexcept
      : ycxx::adl_free::sp_atomic<shared_ptr<T>>(static_cast<shared_ptr<T>&&>(desired)) {}
  atomic(const atomic&) = delete;
  void operator=(const atomic&) = delete;
  constexpr void operator=(shared_ptr<T> desired) noexcept { this->store(static_cast<shared_ptr<T>&&>(desired)); }
  constexpr void operator=(nullptr_t) noexcept { this->store(nullptr); }
};

// [util.smartptr.atomic.weak]
template <class T>
struct atomic<weak_ptr<T>> : ycxx::adl_free::sp_atomic<weak_ptr<T>> {
  using value_type = weak_ptr<T>;

  constexpr atomic() noexcept = default;
  constexpr atomic(weak_ptr<T> desired) noexcept
      : ycxx::adl_free::sp_atomic<weak_ptr<T>>(static_cast<weak_ptr<T>&&>(desired)) {}
  atomic(const atomic&) = delete;
  void operator=(const atomic&) = delete;
  constexpr void operator=(weak_ptr<T> desired) noexcept { this->store(static_cast<weak_ptr<T>&&>(desired)); }
};

} // namespace std
