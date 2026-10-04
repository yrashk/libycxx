// libycxx core: memory_order and the operations behind atomic, atomic_ref and atomic_flag
// ([atomics.order], [atomics.lockfree], [atomics.wait]).
//
// Lock-free types. A type is lock-free when its size is 1, 2, 4, 8 or 16 bytes and the compiler
// reports __atomic_always_lock_free for that size; atomic<T> and atomic_ref<T> then align the
// object to its size. Every operation works on the object as an unsigned integer of the same
// size (a may_alias type, so the access never violates type-based aliasing): values are copied
// into and out of that integer with memcpy, after their padding bits are cleared, so that
// compare-and-exchange compares value representations ([atomics.types.operations]/23, Note 7).
// A compare-and-exchange that fails only because the stored object's padding differs (possible
// through atomic_ref) is retried with the stored bytes.
//
// Other types are lock-based: each operation holds one lock of a striped table of 256 locks
// (selected by the object's address) in the runtime archive (src/runtime/atomic), so libycxx
// needs no libatomic. A lock waits through the PAL (ycxx_pal_wait) after a short spin.
//
// Waiting and notifying go through a second table of 256 address-keyed slots, also in the
// runtime archive: a waiter registers in its slot, re-checks the value and blocks on the slot's
// 32-bit version counter (ycxx_pal_wait); a notification bumps the version and wakes the slot
// only when it has waiters, so notify on an object nobody waits for costs a fence and a load.
// Slots are shared between addresses, so notify_one wakes every waiter of the slot (the others
// re-check their value and block again).
//
// During constant evaluation every operation is a plain access (there is one thread); wait()
// returns at once.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/type_traits.hpp>

namespace std {

// [atomics.order]; consume is [depr.atomics.order].
enum class memory_order : int { relaxed = 0, consume [[deprecated("memory_order::consume is deprecated ([depr.atomics.order]); use acquire")]] = 1, acquire = 2, release = 3, acq_rel = 4, seq_cst = 5 };
inline constexpr memory_order memory_order_relaxed = memory_order::relaxed;
[[deprecated("memory_order_consume is deprecated ([depr.atomics.order]); use memory_order_acquire")]]
inline constexpr memory_order memory_order_consume = static_cast<memory_order>(1);
inline constexpr memory_order memory_order_acquire = memory_order::acquire;
inline constexpr memory_order memory_order_release = memory_order::release;
inline constexpr memory_order memory_order_acq_rel = memory_order::acq_rel;
inline constexpr memory_order memory_order_seq_cst = memory_order::seq_cst;

// [depr.atomics.order]/2
template <class T>
[[deprecated("kill_dependency is deprecated ([depr.atomics.order])")]] constexpr T kill_dependency(T y) noexcept {
  return y;
}

} // namespace std

namespace ycxx::detail {

// ---- the runtime archive (src/runtime/atomic) ----------------------------------------------
// The lock that guards the lock-based atomic object at `addr`.
void atomic_lock(const volatile void* addr) noexcept;
void atomic_unlock(const volatile void* addr) noexcept;
// Waiting on the object at `addr`: prepare registers the caller as a waiter and returns the
// slot's version; the caller then re-checks its condition and either cancels (deregisters) or
// blocks, which returns once the version differs from `ticket` (or spuriously) and deregisters.
std::uint32_t atomic_wait_prepare(const volatile void* addr) noexcept;
void atomic_wait_block(const volatile void* addr, std::uint32_t ticket) noexcept;
void atomic_wait_cancel(const volatile void* addr) noexcept;
// Wakes every waiter registered for the slot of `addr`.
void atomic_notify(const volatile void* addr) noexcept;

// ---- representation -------------------------------------------------------------------------
template <std::size_t N>
struct atomic_rep_impl {};
using atomic_u8 [[gnu::may_alias]] = unsigned char;
using atomic_u16 [[gnu::may_alias]] = unsigned short;
using atomic_u32 [[gnu::may_alias]] = unsigned int;
using atomic_u64 [[gnu::may_alias]] = unsigned long long;
template <>
struct atomic_rep_impl<1> {
  using type = atomic_u8;
};
template <>
struct atomic_rep_impl<2> {
  using type = atomic_u16;
};
template <>
struct atomic_rep_impl<4> {
  using type = atomic_u32;
};
template <>
struct atomic_rep_impl<8> {
  using type = atomic_u64;
};
template <std::size_t N>
  requires(N == 16 && cfg::has_int128)
struct atomic_rep_impl<N> {
  using u128 [[gnu::may_alias]] = uint128;
  using type = u128;
};
template <class V>
using atomic_rep = typename atomic_rep_impl<sizeof(V)>::type;

template <class V>
concept atomic_has_rep = requires { typename atomic_rep<V>; };

template <class V>
inline constexpr bool atomic_lock_free = [] {
  if constexpr (atomic_has_rep<V>)
    return __atomic_always_lock_free(sizeof(V), 0);
  else
    return false;
}();

// The alignment atomic_ref<V> requires: a lock-free V is accessed as its representation type.
template <class V>
inline constexpr std::size_t atomic_align = atomic_lock_free<V> && sizeof(V) > alignof(V) ? sizeof(V) : alignof(V);
// The alignment of atomic<V>'s object: its size whenever that is a representation size, also
// when the type is not lock-free with the current options (for instance a 16-byte type without
// -mcx16), so the layout of atomic<V> does not depend on them.
template <class V>
inline constexpr std::size_t atomic_object_align = atomic_has_rep<V> && sizeof(V) > alignof(V) ? sizeof(V) : alignof(V);

template <class V>
inline constexpr bool atomic_padded = !__has_unique_object_representations(V);

// Uninitialized storage for one V (V is trivially copyable, hence trivially destructible).
template <class V>
union atomic_buf {
  V v;
  constexpr atomic_buf() noexcept {}
};

constexpr int atomic_order(std::memory_order o) noexcept { return static_cast<int>(o); }

// [atomics.types.operations]/23: the failure order of the one-order compare-and-exchange.
constexpr std::memory_order atomic_failure_order(std::memory_order o) noexcept {
  if (o == std::memory_order::acq_rel)
    return std::memory_order::acquire;
  if (o == std::memory_order::release)
    return std::memory_order::relaxed;
  return o;
}
// The builtins want a success order at least as strong as the failure order.
constexpr int atomic_success_order(std::memory_order s, std::memory_order f) noexcept {
  int si = static_cast<int>(s), fi = static_cast<int>(f);
  if (fi == 5)
    return 5;
  if (fi == 1 || fi == 2) { // consume, acquire
    if (si == 0)
      return 2;
    if (si == 3)
      return 4;
  }
  return si;
}

constexpr void atomic_check_load(std::memory_order o) noexcept {
  ::ycxx::detail::precondition(o != std::memory_order::release && o != std::memory_order::acq_rel,
                               "atomic load: order must be relaxed, acquire or seq_cst");
}
constexpr void atomic_check_store(std::memory_order o) noexcept {
  ::ycxx::detail::precondition(o == std::memory_order::relaxed || o == std::memory_order::release ||
                                   o == std::memory_order::seq_cst,
                               "atomic store: order must be relaxed, release or seq_cst");
}

template <class V>
V atomic_from_rep(atomic_rep<V> r) noexcept {
  atomic_buf<V> b;
  __builtin_memcpy(__builtin_addressof(b.v), __builtin_addressof(r), sizeof(V));
  return b.v;
}
template <class V>
atomic_rep<V> atomic_to_rep(const V& v) noexcept {
  atomic_buf<V> b;
  __builtin_memcpy(__builtin_addressof(b.v), __builtin_addressof(v), sizeof(V));
  if constexpr (atomic_padded<V>)
    __builtin_clear_padding(__builtin_addressof(b.v));
  atomic_rep<V> r;
  __builtin_memcpy(__builtin_addressof(r), __builtin_addressof(b.v), sizeof(V));
  return r;
}
// Equality of value representations.
template <class V>
bool atomic_same_value(const V& a, const V& b) noexcept {
  if constexpr (atomic_has_rep<V>) {
    return ::ycxx::detail::atomic_to_rep(a) == ::ycxx::detail::atomic_to_rep(b);
  } else {
    atomic_buf<V> x, y;
    __builtin_memcpy(__builtin_addressof(x.v), __builtin_addressof(a), sizeof(V));
    __builtin_memcpy(__builtin_addressof(y.v), __builtin_addressof(b), sizeof(V));
    if constexpr (atomic_padded<V>) {
      __builtin_clear_padding(__builtin_addressof(x.v));
      __builtin_clear_padding(__builtin_addressof(y.v));
    }
    return __builtin_memcmp(__builtin_addressof(x.v), __builtin_addressof(y.v), sizeof(V)) == 0;
  }
}

// Equality of value representations during constant evaluation (the padding bits of a value
// are indeterminate there, so a padded type cannot be compared).
template <class V>
struct atomic_bytes {
  unsigned char b[sizeof(V)];
};
template <class V>
constexpr bool atomic_const_same(const V& a, const V& b) noexcept {
  const auto x = __builtin_bit_cast(atomic_bytes<V>, a);
  const auto y = __builtin_bit_cast(atomic_bytes<V>, b);
  for (std::size_t i = 0; i != sizeof(V); ++i)
    if (x.b[i] != y.b[i])
      return false;
  return true;
}

// The object as its representation type, with the cv-qualifiers of the pointee.
template <class V, class T>
auto* atomic_rep_ptr(T* p) noexcept {
  return reinterpret_cast<copy_cv<T, atomic_rep<V>>*>(p);
}
// The object as a plain V for the lock-based operations (the lock serializes every access).
template <class T>
auto* atomic_plain_ptr(T* p) noexcept {
  return const_cast<__remove_cv(T)*>(p);
}

// The object for the plain accesses of constant evaluation (never volatile there).
template <class T>
constexpr std::remove_volatile_t<T>* atomic_cx(T* p) noexcept {
  return const_cast<std::remove_volatile_t<T>*>(p);
}

// Holds the lock of a lock-based object.
class atomic_lock_guard {
  const volatile void* addr_;

public:
  explicit atomic_lock_guard(const volatile void* addr) noexcept : addr_(addr) { ::ycxx::detail::atomic_lock(addr); }
  atomic_lock_guard(const atomic_lock_guard&) = delete;
  atomic_lock_guard& operator=(const atomic_lock_guard&) = delete;
  ~atomic_lock_guard() { ::ycxx::detail::atomic_unlock(addr_); }
};

// ---- operations; T is V with the cv-qualifiers of the accessed object -----------------------
template <class V, class T>
constexpr V atomic_load(T* p, std::memory_order o) noexcept {
  ::ycxx::detail::atomic_check_load(o);
  if consteval {
    return *::ycxx::detail::atomic_cx(p);
  } else {
    if constexpr (atomic_lock_free<V>) {
      return ::ycxx::detail::atomic_from_rep<V>(__atomic_load_n(::ycxx::detail::atomic_rep_ptr<V>(p), atomic_order(o)));
    } else {
      atomic_lock_guard g(p);
      return *::ycxx::detail::atomic_plain_ptr(p);
    }
  }
}

template <class V, class T>
constexpr void atomic_store(T* p, const V& v, std::memory_order o) noexcept {
  ::ycxx::detail::atomic_check_store(o);
  if consteval {
    *::ycxx::detail::atomic_cx(p) = v;
  } else {
    if constexpr (atomic_lock_free<V>) {
      __atomic_store_n(::ycxx::detail::atomic_rep_ptr<V>(p), ::ycxx::detail::atomic_to_rep(v), atomic_order(o));
    } else {
      atomic_lock_guard g(p);
      __builtin_memcpy(::ycxx::detail::atomic_plain_ptr(p), __builtin_addressof(v), sizeof(V));
    }
  }
}

template <class V, class T>
constexpr V atomic_exchange(T* p, const V& v, std::memory_order o) noexcept {
  if consteval {
    V old = *::ycxx::detail::atomic_cx(p);
    *::ycxx::detail::atomic_cx(p) = v;
    return old;
  } else {
    if constexpr (atomic_lock_free<V>) {
      return ::ycxx::detail::atomic_from_rep<V>(
          __atomic_exchange_n(::ycxx::detail::atomic_rep_ptr<V>(p), ::ycxx::detail::atomic_to_rep(v), atomic_order(o)));
    } else {
      atomic_lock_guard g(p);
      V* q = ::ycxx::detail::atomic_plain_ptr(p);
      V old = *q;
      __builtin_memcpy(q, __builtin_addressof(v), sizeof(V));
      return old;
    }
  }
}

template <class V, class T>
constexpr bool atomic_compare_exchange(T* p, V& expected, const V& desired, bool weak, std::memory_order s,
                                       std::memory_order f) noexcept {
  ::ycxx::detail::precondition(f != std::memory_order::release && f != std::memory_order::acq_rel,
                               "atomic compare_exchange: failure order must be relaxed, acquire or seq_cst");
  if consteval {
    if (::ycxx::detail::atomic_const_same(*::ycxx::detail::atomic_cx(p), expected)) {
      *::ycxx::detail::atomic_cx(p) = desired;
      return true;
    }
    expected = *::ycxx::detail::atomic_cx(p);
    return false;
  } else {
    if constexpr (atomic_lock_free<V>) {
      using R = atomic_rep<V>;
      auto* ip = ::ycxx::detail::atomic_rep_ptr<V>(p);
      R e = ::ycxx::detail::atomic_to_rep(expected);
      const R d = ::ycxx::detail::atomic_to_rep(desired);
      const int so = ::ycxx::detail::atomic_success_order(s, f);
      for (;;) {
        R cur = e;
        if (__atomic_compare_exchange_n(ip, &cur, d, weak, so, atomic_order(f)))
          return true;
        if constexpr (atomic_padded<V>) {
          // Equal values whose stored padding differs: retry with the stored bytes.
          if (cur != e && ::ycxx::detail::atomic_to_rep(::ycxx::detail::atomic_from_rep<V>(cur)) == e) {
            e = cur;
            continue;
          }
        }
        expected = ::ycxx::detail::atomic_from_rep<V>(cur);
        return false;
      }
    } else {
      atomic_lock_guard g(p);
      V* q = ::ycxx::detail::atomic_plain_ptr(p);
      if (::ycxx::detail::atomic_same_value(*q, expected)) {
        __builtin_memcpy(q, __builtin_addressof(desired), sizeof(V));
        return true;
      }
      __builtin_memcpy(__builtin_addressof(expected), q, sizeof(V));
      return false;
    }
  }
}

// Read-modify-write with an arbitrary computation: a compare-and-exchange loop, or the lock.
template <class V, class T, class F>
constexpr V atomic_rmw(T* p, F f, std::memory_order o) noexcept {
  if consteval {
    V old = *::ycxx::detail::atomic_cx(p);
    *::ycxx::detail::atomic_cx(p) = f(old);
    return old;
  } else {
    if constexpr (atomic_lock_free<V>) {
      V old = ::ycxx::detail::atomic_load<V>(p, std::memory_order::relaxed);
      while (!::ycxx::detail::atomic_compare_exchange<V>(p, old, f(old), true, o, std::memory_order::relaxed)) {
      }
      return old;
    } else {
      atomic_lock_guard g(p);
      V* q = ::ycxx::detail::atomic_plain_ptr(p);
      V old = *q;
      *q = f(old);
      return old;
    }
  }
}

enum class atomic_int_op { add, sub, and_, or_, xor_ };

// fetch_add ... fetch_xor on an integral object (arithmetic modulo 2^N, [atomics.types.int]/8).
template <atomic_int_op Op, class V, class T>
constexpr V atomic_fetch_int(T* p, V arg, std::memory_order o) noexcept {
  if !consteval {
    if constexpr (atomic_lock_free<V>) {
      using R = atomic_rep<V>;
      auto* ip = ::ycxx::detail::atomic_rep_ptr<V>(p);
      const R a = static_cast<R>(arg);
      R r;
      if constexpr (Op == atomic_int_op::add)
        r = __atomic_fetch_add(ip, a, atomic_order(o));
      else if constexpr (Op == atomic_int_op::sub)
        r = __atomic_fetch_sub(ip, a, atomic_order(o));
      else if constexpr (Op == atomic_int_op::and_)
        r = __atomic_fetch_and(ip, a, atomic_order(o));
      else if constexpr (Op == atomic_int_op::or_)
        r = __atomic_fetch_or(ip, a, atomic_order(o));
      else
        r = __atomic_fetch_xor(ip, a, atomic_order(o));
      return static_cast<V>(r);
    }
  }
  return ::ycxx::detail::atomic_rmw<V>(
      p,
      [arg](V v) {
        using U = std::make_unsigned_t<V>;
        if constexpr (Op == atomic_int_op::add)
          return static_cast<V>(static_cast<U>(v) + static_cast<U>(arg));
        else if constexpr (Op == atomic_int_op::sub)
          return static_cast<V>(static_cast<U>(v) - static_cast<U>(arg));
        else if constexpr (Op == atomic_int_op::and_)
          return static_cast<V>(v & arg);
        else if constexpr (Op == atomic_int_op::or_)
          return static_cast<V>(v | arg);
        else
          return static_cast<V>(v ^ arg);
      },
      o);
}

// fetch_add / fetch_sub on a pointer object: n elements of type E.
template <class V, class T>
constexpr V atomic_fetch_ptr(T* p, std::ptrdiff_t n, std::memory_order o) noexcept {
  using E = std::remove_pointer_t<V>;
  static_assert(std::is_object_v<E> && sizeof(E) > 0, "atomic pointer arithmetic needs a pointer to a complete object type");
  if consteval {
    V old = *::ycxx::detail::atomic_cx(p);
    *::ycxx::detail::atomic_cx(p) = old + n;
    return old;
  } else {
    // [atomics.types.pointer]/2: the result may be an invalid pointer value; computed on the
    // integer representation, so no arithmetic on an invalid pointer is performed.
    using R = atomic_rep<V>;
    const R bytes = static_cast<R>(n) * static_cast<R>(sizeof(E));
    if constexpr (atomic_lock_free<V>) {
      return ::ycxx::detail::atomic_from_rep<V>(
          __atomic_fetch_add(::ycxx::detail::atomic_rep_ptr<V>(p), bytes, atomic_order(o)));
    } else {
      return ::ycxx::detail::atomic_rmw<V>(
          p, [bytes](V v) { return reinterpret_cast<V>(reinterpret_cast<__UINTPTR_TYPE__>(v) + bytes); }, o);
    }
  }
}

// [atomics.types.int]/9, [atomics.types.pointer]/10: max/min as by std::max/std::min.
template <class V, class T>
constexpr V atomic_fetch_max(T* p, V arg, std::memory_order o) noexcept {
  return ::ycxx::detail::atomic_rmw<V>(p, [arg](V v) { return v < arg ? arg : v; }, o);
}
template <class V, class T>
constexpr V atomic_fetch_min(T* p, V arg, std::memory_order o) noexcept {
  return ::ycxx::detail::atomic_rmw<V>(p, [arg](V v) { return arg < v ? arg : v; }, o);
}

// ---- floating-point maximum/minimum ([atomics.types.float]/9) --------------------------------
template <class V>
constexpr bool fp_isnan(V v) noexcept {
  return v != v;
}
// fmaximum / fminimum: a NaN operand gives NaN; -0 < +0.
template <class V>
constexpr V fp_maximum(V x, V y) noexcept {
  if (fp_isnan(x) || fp_isnan(y))
    return x + y;
  if (x == y)
    return __builtin_signbit(x) ? y : x;
  return x < y ? y : x;
}
template <class V>
constexpr V fp_minimum(V x, V y) noexcept {
  if (fp_isnan(x) || fp_isnan(y))
    return x + y;
  if (x == y)
    return __builtin_signbit(x) ? x : y;
  return x < y ? x : y;
}
// fmaximum_num / fminimum_num: a NaN operand is ignored unless both are NaN.
template <class V>
constexpr V fp_maximum_num(V x, V y) noexcept {
  if (fp_isnan(x))
    return fp_isnan(y) ? x + y : y;
  if (fp_isnan(y))
    return x;
  return ::ycxx::detail::fp_maximum(x, y);
}
template <class V>
constexpr V fp_minimum_num(V x, V y) noexcept {
  if (fp_isnan(x))
    return fp_isnan(y) ? x + y : y;
  if (fp_isnan(y))
    return x;
  return ::ycxx::detail::fp_minimum(x, y);
}

enum class atomic_fp_op { add, sub, maximum, minimum, maximum_num, minimum_num };

template <atomic_fp_op Op, class V, class T>
constexpr V atomic_fetch_fp(T* p, V arg, std::memory_order o) noexcept {
  return ::ycxx::detail::atomic_rmw<V>(
      p,
      [arg](V v) {
        if constexpr (Op == atomic_fp_op::add)
          return static_cast<V>(v + arg);
        else if constexpr (Op == atomic_fp_op::sub)
          return static_cast<V>(v - arg);
        else if constexpr (Op == atomic_fp_op::maximum)
          return ::ycxx::detail::fp_maximum(v, arg);
        else if constexpr (Op == atomic_fp_op::minimum)
          return ::ycxx::detail::fp_minimum(v, arg);
        else if constexpr (Op == atomic_fp_op::maximum_num)
          return ::ycxx::detail::fp_maximum_num(v, arg);
        else
          return ::ycxx::detail::fp_minimum_num(v, arg);
      },
      o);
}

// ---- waiting ----------------------------------------------------------------------------------
// Blocks until done() is true. done() is re-evaluated after every wake-up, spurious or not.
template <class Done>
void atomic_wait_until_done(const volatile void* addr, Done done) noexcept {
  for (int i = 0; i < 32; ++i)
    if (done())
      return;
  for (;;) {
    const std::uint32_t ticket = ::ycxx::detail::atomic_wait_prepare(addr);
    if (done()) {
      ::ycxx::detail::atomic_wait_cancel(addr);
      return;
    }
    ::ycxx::detail::atomic_wait_block(addr, ticket);
    if (done())
      return;
  }
}

template <class V, class T>
constexpr void atomic_wait(T* p, const V& old, std::memory_order o) noexcept {
  ::ycxx::detail::atomic_check_load(o);
  if !consteval {
    ::ycxx::detail::atomic_wait_until_done(
        p, [&] { return !::ycxx::detail::atomic_same_value(::ycxx::detail::atomic_load<V>(p, o), old); });
  }
}

constexpr void atomic_notify_all(const volatile void* addr) noexcept {
  if !consteval {
    ::ycxx::detail::atomic_notify(addr);
  }
}

} // namespace ycxx::detail
