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

namespace [[__gnu__::__visibility__("hidden")]] std {

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
template <class _Tp>
[[deprecated("kill_dependency is deprecated ([depr.atomics.order])")]] constexpr _Tp kill_dependency(_Tp y) noexcept {
  return y;
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// ---- the runtime archive (src/runtime/atomic) ----------------------------------------------
// The lock that guards the lock-based atomic object at `__addr`.
void __atomic_lock(const volatile void* __addr) noexcept;
void __atomic_unlock(const volatile void* __addr) noexcept;
// Waiting on the object at `__addr`: prepare registers the caller as a waiter and returns the
// slot's version; the caller then re-checks its condition and either cancels (deregisters) or
// blocks, which returns once the version differs from `__ticket` (or spuriously) and deregisters.
std::uint32_t __atomic_wait_prepare(const volatile void* __addr) noexcept;
void __atomic_wait_block(const volatile void* __addr, std::uint32_t __ticket) noexcept;
void __atomic_wait_cancel(const volatile void* __addr) noexcept;
// Wakes every waiter registered for the slot of `__addr`.
void __atomic_notify(const volatile void* __addr) noexcept;

// ---- representation -------------------------------------------------------------------------
template <std::size_t _Np>
struct __atomic_rep_impl {};
using __atomic_u8 [[__gnu__::__may_alias__]] = unsigned char;
using __atomic_u16 [[__gnu__::__may_alias__]] = unsigned short;
using __atomic_u32 [[__gnu__::__may_alias__]] = unsigned int;
using __atomic_u64 [[__gnu__::__may_alias__]] = unsigned long long;
template <>
struct __atomic_rep_impl<1> {
  using type = __atomic_u8;
};
template <>
struct __atomic_rep_impl<2> {
  using type = __atomic_u16;
};
template <>
struct __atomic_rep_impl<4> {
  using type = __atomic_u32;
};
template <>
struct __atomic_rep_impl<8> {
  using type = __atomic_u64;
};
template <std::size_t _Np>
  requires(_Np == 16 && __cfg::__has_int128)
struct __atomic_rep_impl<_Np> {
  using __u128 [[__gnu__::__may_alias__]] = __uint128;
  using type = __u128;
};
template <class _Vp>
using __atomic_rep = typename __atomic_rep_impl<sizeof(_Vp)>::type;

template <class _Vp>
concept __atomic_has_rep = requires { typename __atomic_rep<_Vp>; };

template <class _Vp>
inline constexpr bool __atomic_lock_free = [] {
  if constexpr (__atomic_has_rep<_Vp>)
    return __atomic_always_lock_free(sizeof(_Vp), 0);
  else
    return false;
}();

// The alignment atomic_ref<V> requires: a lock-free V is accessed as its representation type.
template <class _Vp>
inline constexpr std::size_t __atomic_align = __atomic_lock_free<_Vp> && sizeof(_Vp) > alignof(_Vp) ? sizeof(_Vp) : alignof(_Vp);
// The alignment of atomic<V>'s object: its size whenever that is a representation size, also
// when the type is not lock-free with the current options (for instance a 16-byte type without
// -mcx16), so the layout of atomic<V> does not depend on them.
template <class _Vp>
inline constexpr std::size_t __atomic_object_align = __atomic_has_rep<_Vp> && sizeof(_Vp) > alignof(_Vp) ? sizeof(_Vp) : alignof(_Vp);

template <class _Vp>
inline constexpr bool __atomic_padded = !__has_unique_object_representations(_Vp);

// Uninitialized storage for one V (V is trivially copyable, hence trivially destructible).
template <class _Vp>
union __atomic_buf {
  _Vp __v;
  constexpr __atomic_buf() noexcept {}
};

constexpr int __atomic_order(std::memory_order __o) noexcept { return static_cast<int>(__o); }

// [atomics.types.operations]/23: the failure order of the one-order compare-and-exchange.
constexpr std::memory_order __atomic_failure_order(std::memory_order __o) noexcept {
  if (__o == std::memory_order::acq_rel)
    return std::memory_order::acquire;
  if (__o == std::memory_order::release)
    return std::memory_order::relaxed;
  return __o;
}
// The builtins want a success order at least as strong as the failure order.
constexpr int __atomic_success_order(std::memory_order s, std::memory_order __f) noexcept {
  int __si = static_cast<int>(s), __fi = static_cast<int>(__f);
  if (__fi == 5)
    return 5;
  if (__fi == 1 || __fi == 2) { // consume, acquire
    if (__si == 0)
      return 2;
    if (__si == 3)
      return 4;
  }
  return __si;
}

constexpr void __atomic_check_load(std::memory_order __o) noexcept {
  ::__ycxx::__detail::__precondition(__o != std::memory_order::release && __o != std::memory_order::acq_rel,
                               "atomic load: order must be relaxed, acquire or seq_cst");
}
constexpr void __atomic_check_store(std::memory_order __o) noexcept {
  ::__ycxx::__detail::__precondition(__o == std::memory_order::relaxed || __o == std::memory_order::release ||
                                   __o == std::memory_order::seq_cst,
                               "atomic store: order must be relaxed, release or seq_cst");
}

template <class _Vp>
_Vp __atomic_from_rep(__atomic_rep<_Vp> r) noexcept {
  __atomic_buf<_Vp> b;
  __builtin_memcpy(__builtin_addressof(b.__v), __builtin_addressof(r), sizeof(_Vp));
  return b.__v;
}
template <class _Vp>
__atomic_rep<_Vp> __atomic_to_rep(const _Vp& __v) noexcept {
  __atomic_buf<_Vp> b;
  __builtin_memcpy(__builtin_addressof(b.__v), __builtin_addressof(__v), sizeof(_Vp));
  if constexpr (__atomic_padded<_Vp>)
    __builtin_clear_padding(__builtin_addressof(b.__v));
  __atomic_rep<_Vp> r;
  __builtin_memcpy(__builtin_addressof(r), __builtin_addressof(b.__v), sizeof(_Vp));
  return r;
}
// Equality of value representations.
template <class _Vp>
bool __atomic_same_value(const _Vp& a, const _Vp& b) noexcept {
  if constexpr (__atomic_has_rep<_Vp>) {
    return ::__ycxx::__detail::__atomic_to_rep(a) == ::__ycxx::__detail::__atomic_to_rep(b);
  } else {
    __atomic_buf<_Vp> __x, y;
    __builtin_memcpy(__builtin_addressof(__x.__v), __builtin_addressof(a), sizeof(_Vp));
    __builtin_memcpy(__builtin_addressof(y.__v), __builtin_addressof(b), sizeof(_Vp));
    if constexpr (__atomic_padded<_Vp>) {
      __builtin_clear_padding(__builtin_addressof(__x.__v));
      __builtin_clear_padding(__builtin_addressof(y.__v));
    }
    return __builtin_memcmp(__builtin_addressof(__x.__v), __builtin_addressof(y.__v), sizeof(_Vp)) == 0;
  }
}

// Equality of value representations during constant evaluation (the padding bits of a value
// are indeterminate there, so a padded type cannot be compared). The x87 80-bit format (64
// significand digits, integer bit explicit) keeps its value in the first 10 bytes of its 12 or
// 16; the bytes after them are padding and are not read ([atomics.ref.float],
// [atomics.types.float]: compare_exchange of a long double in constant evaluation).
template <class _Vp>
struct __atomic_bytes {
  unsigned char b[sizeof(_Vp)];
};
template <class _Vp>
inline constexpr std::size_t __atomic_value_bytes =
    std::is_floating_point_v<_Vp> && __fp_format<std::remove_cv_t<_Vp>>.digits == 64 && sizeof(_Vp) > 10 ? 10 : sizeof(_Vp);
template <class _Vp>
constexpr bool __atomic_const_same(const _Vp& a, const _Vp& b) noexcept {
  const auto __x = __builtin_bit_cast(__atomic_bytes<_Vp>, a);
  const auto y = __builtin_bit_cast(__atomic_bytes<_Vp>, b);
  for (std::size_t i = 0; i != __atomic_value_bytes<_Vp>; ++i)
    if (__x.b[i] != y.b[i])
      return false;
  return true;
}

// The object as its representation type, with the cv-qualifiers of the pointee.
template <class _Vp, class _Tp>
auto* __atomic_rep_ptr(_Tp* p) noexcept {
  return reinterpret_cast<__copy_cv<_Tp, __atomic_rep<_Vp>>*>(p);
}
// The object as a plain V for the lock-based operations (the lock serializes every access).
template <class _Tp>
auto* __atomic_plain_ptr(_Tp* p) noexcept {
  return const_cast<__remove_cv(_Tp)*>(p);
}

// The object for the plain accesses of constant evaluation (never volatile there).
template <class _Tp>
constexpr std::remove_volatile_t<_Tp>* __atomic_cx(_Tp* p) noexcept {
  return const_cast<std::remove_volatile_t<_Tp>*>(p);
}

// Holds the lock of a lock-based object.
class __atomic_lock_guard {
  const volatile void* __addr_;

public:
  explicit __atomic_lock_guard(const volatile void* __addr) noexcept : __addr_(__addr) { ::__ycxx::__detail::__atomic_lock(__addr); }
  __atomic_lock_guard(const __atomic_lock_guard&) = delete;
  __atomic_lock_guard& operator=(const __atomic_lock_guard&) = delete;
  ~__atomic_lock_guard() { ::__ycxx::__detail::__atomic_unlock(__addr_); }
};

// ---- operations; T is V with the cv-qualifiers of the accessed object -----------------------
template <class _Vp, class _Tp>
constexpr _Vp atomic_load(_Tp* p, std::memory_order __o) noexcept {
  ::__ycxx::__detail::__atomic_check_load(__o);
  if consteval {
    return *::__ycxx::__detail::__atomic_cx(p);
  } else {
    if constexpr (__atomic_lock_free<_Vp>) {
      return ::__ycxx::__detail::__atomic_from_rep<_Vp>(__atomic_load_n(::__ycxx::__detail::__atomic_rep_ptr<_Vp>(p), __atomic_order(__o)));
    } else {
      __atomic_lock_guard __g(p);
      return *::__ycxx::__detail::__atomic_plain_ptr(p);
    }
  }
}

template <class _Vp, class _Tp>
constexpr void atomic_store(_Tp* p, const _Vp& __v, std::memory_order __o) noexcept {
  ::__ycxx::__detail::__atomic_check_store(__o);
  if consteval {
    *::__ycxx::__detail::__atomic_cx(p) = __v;
  } else {
    if constexpr (__atomic_lock_free<_Vp>) {
      __atomic_store_n(::__ycxx::__detail::__atomic_rep_ptr<_Vp>(p), ::__ycxx::__detail::__atomic_to_rep(__v), __atomic_order(__o));
    } else {
      __atomic_lock_guard __g(p);
      __builtin_memcpy(::__ycxx::__detail::__atomic_plain_ptr(p), __builtin_addressof(__v), sizeof(_Vp));
    }
  }
}

template <class _Vp, class _Tp>
constexpr _Vp atomic_exchange(_Tp* p, const _Vp& __v, std::memory_order __o) noexcept {
  if consteval {
    _Vp __old = *::__ycxx::__detail::__atomic_cx(p);
    *::__ycxx::__detail::__atomic_cx(p) = __v;
    return __old;
  } else {
    if constexpr (__atomic_lock_free<_Vp>) {
      return ::__ycxx::__detail::__atomic_from_rep<_Vp>(
          __atomic_exchange_n(::__ycxx::__detail::__atomic_rep_ptr<_Vp>(p), ::__ycxx::__detail::__atomic_to_rep(__v), __atomic_order(__o)));
    } else {
      __atomic_lock_guard __g(p);
      _Vp* __q = ::__ycxx::__detail::__atomic_plain_ptr(p);
      _Vp __old = *__q;
      __builtin_memcpy(__q, __builtin_addressof(__v), sizeof(_Vp));
      return __old;
    }
  }
}

template <class _Vp, class _Tp>
constexpr bool __y_atomic_compare_exchange(_Tp* p, _Vp& expected, const _Vp& __desired, bool __y_weak, std::memory_order s,
                                       std::memory_order __f) noexcept {
  ::__ycxx::__detail::__precondition(__f != std::memory_order::release && __f != std::memory_order::acq_rel,
                               "atomic compare_exchange: failure order must be relaxed, acquire or seq_cst");
  if consteval {
    if (::__ycxx::__detail::__atomic_const_same(*::__ycxx::__detail::__atomic_cx(p), expected)) {
      *::__ycxx::__detail::__atomic_cx(p) = __desired;
      return true;
    }
    expected = *::__ycxx::__detail::__atomic_cx(p);
    return false;
  } else {
    if constexpr (__atomic_lock_free<_Vp>) {
      using _Rp = __atomic_rep<_Vp>;
      auto* __ip = ::__ycxx::__detail::__atomic_rep_ptr<_Vp>(p);
      _Rp e = ::__ycxx::__detail::__atomic_to_rep(expected);
      const _Rp d = ::__ycxx::__detail::__atomic_to_rep(__desired);
      const int __so = ::__ycxx::__detail::__atomic_success_order(s, __f);
      for (;;) {
        _Rp cur = e;
        if (__atomic_compare_exchange_n(__ip, &cur, d, __y_weak, __so, __atomic_order(__f)))
          return true;
        if constexpr (__atomic_padded<_Vp>) {
          // Equal values whose stored padding differs: retry with the stored bytes.
          if (cur != e && ::__ycxx::__detail::__atomic_to_rep(::__ycxx::__detail::__atomic_from_rep<_Vp>(cur)) == e) {
            e = cur;
            continue;
          }
        }
        expected = ::__ycxx::__detail::__atomic_from_rep<_Vp>(cur);
        return false;
      }
    } else {
      __atomic_lock_guard __g(p);
      _Vp* __q = ::__ycxx::__detail::__atomic_plain_ptr(p);
      if (::__ycxx::__detail::__atomic_same_value(*__q, expected)) {
        __builtin_memcpy(__q, __builtin_addressof(__desired), sizeof(_Vp));
        return true;
      }
      __builtin_memcpy(__builtin_addressof(expected), __q, sizeof(_Vp));
      return false;
    }
  }
}

// Read-modify-write with an arbitrary computation: a compare-and-exchange loop, or the lock.
template <class _Vp, class _Tp, class _Fp>
constexpr _Vp __atomic_rmw(_Tp* p, _Fp __f, std::memory_order __o) noexcept {
  if consteval {
    _Vp __old = *::__ycxx::__detail::__atomic_cx(p);
    *::__ycxx::__detail::__atomic_cx(p) = __f(__old);
    return __old;
  } else {
    if constexpr (__atomic_lock_free<_Vp>) {
      _Vp __old = ::__ycxx::__detail::atomic_load<_Vp>(p, std::memory_order::relaxed);
      while (!::__ycxx::__detail::__y_atomic_compare_exchange<_Vp>(p, __old, __f(__old), true, __o, std::memory_order::relaxed)) {
      }
      return __old;
    } else {
      __atomic_lock_guard __g(p);
      _Vp* __q = ::__ycxx::__detail::__atomic_plain_ptr(p);
      _Vp __old = *__q;
      *__q = __f(__old);
      return __old;
    }
  }
}

enum class __atomic_int_op { add, __sub, __and_, __or_, __xor_ };

// fetch_add ... fetch_xor on an integral object (arithmetic modulo 2^N, [atomics.types.int]/8).
template <__atomic_int_op _Op_, class _Vp, class _Tp>
constexpr _Vp __atomic_fetch_int(_Tp* p, _Vp arg, std::memory_order __o) noexcept {
  if !consteval {
    if constexpr (__atomic_lock_free<_Vp>) {
      using _Rp = __atomic_rep<_Vp>;
      auto* __ip = ::__ycxx::__detail::__atomic_rep_ptr<_Vp>(p);
      const _Rp a = static_cast<_Rp>(arg);
      _Rp r;
      if constexpr (_Op_ == __atomic_int_op::add)
        r = __atomic_fetch_add(__ip, a, __atomic_order(__o));
      else if constexpr (_Op_ == __atomic_int_op::__sub)
        r = __atomic_fetch_sub(__ip, a, __atomic_order(__o));
      else if constexpr (_Op_ == __atomic_int_op::__and_)
        r = __atomic_fetch_and(__ip, a, __atomic_order(__o));
      else if constexpr (_Op_ == __atomic_int_op::__or_)
        r = __atomic_fetch_or(__ip, a, __atomic_order(__o));
      else
        r = __atomic_fetch_xor(__ip, a, __atomic_order(__o));
      return static_cast<_Vp>(r);
    }
  }
  return ::__ycxx::__detail::__atomic_rmw<_Vp>(
      p,
      [arg](_Vp __v) {
        using _Up = std::make_unsigned_t<_Vp>;
        if constexpr (_Op_ == __atomic_int_op::add)
          return static_cast<_Vp>(static_cast<_Up>(__v) + static_cast<_Up>(arg));
        else if constexpr (_Op_ == __atomic_int_op::__sub)
          return static_cast<_Vp>(static_cast<_Up>(__v) - static_cast<_Up>(arg));
        else if constexpr (_Op_ == __atomic_int_op::__and_)
          return static_cast<_Vp>(__v & arg);
        else if constexpr (_Op_ == __atomic_int_op::__or_)
          return static_cast<_Vp>(__v | arg);
        else
          return static_cast<_Vp>(__v ^ arg);
      },
      __o);
}

// fetch_add / fetch_sub on a pointer object: n elements of type E.
template <class _Vp, class _Tp>
constexpr _Vp __atomic_fetch_ptr(_Tp* p, std::ptrdiff_t n, std::memory_order __o) noexcept {
  using _Ep = std::remove_pointer_t<_Vp>;
  static_assert(std::is_object_v<_Ep> && sizeof(_Ep) > 0, "atomic pointer arithmetic needs a pointer to a complete object type");
  if consteval {
    _Vp __old = *::__ycxx::__detail::__atomic_cx(p);
    *::__ycxx::__detail::__atomic_cx(p) = __old + n;
    return __old;
  } else {
    // [atomics.types.pointer]/2: the result may be an invalid pointer value; computed on the
    // integer representation, so no arithmetic on an invalid pointer is performed.
    using _Rp = __atomic_rep<_Vp>;
    const _Rp bytes = static_cast<_Rp>(n) * static_cast<_Rp>(sizeof(_Ep));
    if constexpr (__atomic_lock_free<_Vp>) {
      return ::__ycxx::__detail::__atomic_from_rep<_Vp>(
          __atomic_fetch_add(::__ycxx::__detail::__atomic_rep_ptr<_Vp>(p), bytes, __atomic_order(__o)));
    } else {
      return ::__ycxx::__detail::__atomic_rmw<_Vp>(
          p, [bytes](_Vp __v) { return reinterpret_cast<_Vp>(reinterpret_cast<__UINTPTR_TYPE__>(__v) + bytes); }, __o);
    }
  }
}

// [atomics.types.int]/9, [atomics.types.pointer]/10: max/min as by std::max/std::min.
template <class _Vp, class _Tp>
constexpr _Vp atomic_fetch_max(_Tp* p, _Vp arg, std::memory_order __o) noexcept {
  return ::__ycxx::__detail::__atomic_rmw<_Vp>(p, [arg](_Vp __v) { return __v < arg ? arg : __v; }, __o);
}
template <class _Vp, class _Tp>
constexpr _Vp atomic_fetch_min(_Tp* p, _Vp arg, std::memory_order __o) noexcept {
  return ::__ycxx::__detail::__atomic_rmw<_Vp>(p, [arg](_Vp __v) { return arg < __v ? arg : __v; }, __o);
}

// ---- floating-point maximum/minimum ([atomics.types.float]/9) --------------------------------
template <class _Vp>
constexpr bool __fp_isnan(_Vp __v) noexcept {
  return __v != __v;
}
// fmaximum / fminimum: a NaN operand gives NaN; -0 < +0.
template <class _Vp>
constexpr _Vp __fp_maximum(_Vp __x, _Vp y) noexcept {
  if (__fp_isnan(__x) || __fp_isnan(y))
    return __x + y;
  if (__x == y)
    return __builtin_signbit(__x) ? y : __x;
  return __x < y ? y : __x;
}
template <class _Vp>
constexpr _Vp __fp_minimum(_Vp __x, _Vp y) noexcept {
  if (__fp_isnan(__x) || __fp_isnan(y))
    return __x + y;
  if (__x == y)
    return __builtin_signbit(__x) ? __x : y;
  return __x < y ? __x : y;
}
// fmaximum_num / fminimum_num: a NaN operand is ignored unless both are NaN.
template <class _Vp>
constexpr _Vp __fp_maximum_num(_Vp __x, _Vp y) noexcept {
  if (__fp_isnan(__x))
    return __fp_isnan(y) ? __x + y : y;
  if (__fp_isnan(y))
    return __x;
  return ::__ycxx::__detail::__fp_maximum(__x, y);
}
template <class _Vp>
constexpr _Vp __fp_minimum_num(_Vp __x, _Vp y) noexcept {
  if (__fp_isnan(__x))
    return __fp_isnan(y) ? __x + y : y;
  if (__fp_isnan(y))
    return __x;
  return ::__ycxx::__detail::__fp_minimum(__x, y);
}

enum class __atomic_fp_op { add, __sub, __maximum, __minimum, __maximum_num, __minimum_num };

template <__atomic_fp_op _Op_, class _Vp, class _Tp>
constexpr _Vp __atomic_fetch_fp(_Tp* p, _Vp arg, std::memory_order __o) noexcept {
  return ::__ycxx::__detail::__atomic_rmw<_Vp>(
      p,
      [arg](_Vp __v) {
        if constexpr (_Op_ == __atomic_fp_op::add)
          return static_cast<_Vp>(__v + arg);
        else if constexpr (_Op_ == __atomic_fp_op::__sub)
          return static_cast<_Vp>(__v - arg);
        else if constexpr (_Op_ == __atomic_fp_op::__maximum)
          return ::__ycxx::__detail::__fp_maximum(__v, arg);
        else if constexpr (_Op_ == __atomic_fp_op::__minimum)
          return ::__ycxx::__detail::__fp_minimum(__v, arg);
        else if constexpr (_Op_ == __atomic_fp_op::__maximum_num)
          return ::__ycxx::__detail::__fp_maximum_num(__v, arg);
        else
          return ::__ycxx::__detail::__fp_minimum_num(__v, arg);
      },
      __o);
}

// ---- waiting ----------------------------------------------------------------------------------
// Blocks until done() is true. done() is re-evaluated after every wake-up, spurious or not.
template <class _Done>
void __atomic_wait_until_done(const volatile void* __addr, _Done done) noexcept {
  for (int i = 0; i < 32; ++i)
    if (done())
      return;
  for (;;) {
    const std::uint32_t __ticket = ::__ycxx::__detail::__atomic_wait_prepare(__addr);
    if (done()) {
      ::__ycxx::__detail::__atomic_wait_cancel(__addr);
      return;
    }
    ::__ycxx::__detail::__atomic_wait_block(__addr, __ticket);
    if (done())
      return;
  }
}

template <class _Vp, class _Tp>
constexpr void atomic_wait(_Tp* p, const _Vp& __old, std::memory_order __o) noexcept {
  ::__ycxx::__detail::__atomic_check_load(__o);
  if !consteval {
    ::__ycxx::__detail::__atomic_wait_until_done(
        p, [&] { return !::__ycxx::__detail::__atomic_same_value(::__ycxx::__detail::atomic_load<_Vp>(p, __o), __old); });
  }
}

constexpr void atomic_notify_all(const volatile void* __addr) noexcept {
  if !consteval {
    ::__ycxx::__detail::__atomic_notify(__addr);
  }
}

}} // namespace __ycxx::__detail
