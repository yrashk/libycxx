// [stdatomic.h.syn]: "#define _Atomic(T) std-atomic<T>" where std-atomic<T> is std::atomic<T>;
// /1: "Each using-declaration for some name A in the synopsis above makes available the same
// entity as std::A declared in <atomic>. Each macro listed above other than _Atomic(T) is
// defined as in <atomic>." /2: atomic_intN_t etc. iff <atomic> declares them. Usable as in C:
// the C-style free functions, memory_order_* constants, atomic_flag with ATOMIC_FLAG_INIT, and
// the fences, all in the global namespace.
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <stdatomic.h>
#include <atomic>
#include <cstdint>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<_Atomic(int), std::atomic<int>>);
static_assert(std::is_same_v<_Atomic(unsigned long long), std::atomic<unsigned long long>>);
static_assert(std::is_same_v<_Atomic(int*), std::atomic<int*>>);
struct S { int a, b; };
static_assert(std::is_same_v<_Atomic(S), std::atomic<S>>);

static_assert(std::is_same_v<::memory_order, std::memory_order>);
static_assert(::memory_order_relaxed == std::memory_order_relaxed);
static_assert(::memory_order_acquire == std::memory_order::acquire);
static_assert(::memory_order_release == std::memory_order::release);
static_assert(::memory_order_acq_rel == std::memory_order::acq_rel);
static_assert(::memory_order_seq_cst == std::memory_order::seq_cst);
static_assert(&::memory_order_seq_cst == &std::memory_order_seq_cst);  // the same entity

static_assert(std::is_same_v<::atomic_flag, std::atomic_flag>);
static_assert(std::is_same_v<::atomic_bool, std::atomic_bool>);
static_assert(std::is_same_v<::atomic_char, std::atomic<char>>);
static_assert(std::is_same_v<::atomic_schar, std::atomic<signed char>>);
static_assert(std::is_same_v<::atomic_uchar, std::atomic<unsigned char>>);
static_assert(std::is_same_v<::atomic_short, std::atomic<short>>);
static_assert(std::is_same_v<::atomic_ushort, std::atomic<unsigned short>>);
static_assert(std::is_same_v<::atomic_int, std::atomic<int>>);
static_assert(std::is_same_v<::atomic_uint, std::atomic<unsigned>>);
static_assert(std::is_same_v<::atomic_long, std::atomic<long>>);
static_assert(std::is_same_v<::atomic_ulong, std::atomic<unsigned long>>);
static_assert(std::is_same_v<::atomic_llong, std::atomic<long long>>);
static_assert(std::is_same_v<::atomic_ullong, std::atomic<unsigned long long>>);
static_assert(std::is_same_v<::atomic_char8_t, std::atomic<char8_t>>);
static_assert(std::is_same_v<::atomic_char16_t, std::atomic<char16_t>>);
static_assert(std::is_same_v<::atomic_char32_t, std::atomic<char32_t>>);
static_assert(std::is_same_v<::atomic_wchar_t, std::atomic<wchar_t>>);
static_assert(std::is_same_v<::atomic_int_least8_t, std::atomic_int_least8_t>);
static_assert(std::is_same_v<::atomic_uint_least64_t, std::atomic_uint_least64_t>);
static_assert(std::is_same_v<::atomic_int_fast16_t, std::atomic_int_fast16_t>);
static_assert(std::is_same_v<::atomic_uint_fast32_t, std::atomic_uint_fast32_t>);
static_assert(std::is_same_v<::atomic_size_t, std::atomic<std::size_t>>);
static_assert(std::is_same_v<::atomic_ptrdiff_t, std::atomic<std::ptrdiff_t>>);
static_assert(std::is_same_v<::atomic_intmax_t, std::atomic<std::intmax_t>>);
static_assert(std::is_same_v<::atomic_uintmax_t, std::atomic<std::uintmax_t>>);
#ifdef INT32_MAX  // /2: iff the implementation declares the typedef-names (glibc does)
static_assert(std::is_same_v<::atomic_int8_t, std::atomic<std::int8_t>>);
static_assert(std::is_same_v<::atomic_uint16_t, std::atomic<std::uint16_t>>);
static_assert(std::is_same_v<::atomic_int32_t, std::atomic<std::int32_t>>);
static_assert(std::is_same_v<::atomic_uint64_t, std::atomic<std::uint64_t>>);
#endif
#ifdef INTPTR_MAX
static_assert(std::is_same_v<::atomic_intptr_t, std::atomic<std::intptr_t>>);
static_assert(std::is_same_v<::atomic_uintptr_t, std::atomic<std::uintptr_t>>);
#endif

// the macros, as in <atomic>
static_assert(ATOMIC_BOOL_LOCK_FREE >= 0 && ATOMIC_BOOL_LOCK_FREE <= 2);
static_assert(ATOMIC_CHAR_LOCK_FREE >= 0 && ATOMIC_CHAR8_T_LOCK_FREE >= 0 && ATOMIC_CHAR16_T_LOCK_FREE >= 0);
static_assert(ATOMIC_CHAR32_T_LOCK_FREE >= 0 && ATOMIC_WCHAR_T_LOCK_FREE >= 0 && ATOMIC_SHORT_LOCK_FREE >= 0);
static_assert(ATOMIC_INT_LOCK_FREE >= 0 && ATOMIC_LONG_LOCK_FREE >= 0 && ATOMIC_LLONG_LOCK_FREE >= 0);
static_assert(ATOMIC_POINTER_LOCK_FREE >= 0 && ATOMIC_POINTER_LOCK_FREE <= 2);
static_assert(ATOMIC_INT_LOCK_FREE != 2 || std::atomic<int>::is_always_lock_free);  // [atomics.lockfree]/1

// "C" code using the facilities
static ::atomic_flag flag = ATOMIC_FLAG_INIT;
static _Atomic(int) counter(0);

int main() {
  ::atomic_int x(1);
  CHECK(::atomic_is_lock_free(&x) == x.is_lock_free());
  CHECK(::atomic_load(&x) == 1);
  ::atomic_store(&x, 2);
  CHECK(::atomic_load_explicit(&x, memory_order_acquire) == 2);
  ::atomic_store_explicit(&x, 3, memory_order_release);
  CHECK(::atomic_exchange(&x, 4) == 3);
  CHECK(::atomic_exchange_explicit(&x, 5, memory_order_acq_rel) == 4);
  int e = 0;
  CHECK(!::atomic_compare_exchange_strong(&x, &e, 6));
  CHECK(e == 5);
  CHECK(::atomic_compare_exchange_strong_explicit(&x, &e, 6, memory_order_seq_cst, memory_order_relaxed));
  e = 6;
  while (!::atomic_compare_exchange_weak(&x, &e, 7)) CHECK(e == 6);
  e = 7;
  while (!::atomic_compare_exchange_weak_explicit(&x, &e, 0b1100, memory_order_seq_cst, memory_order_relaxed))
    CHECK(e == 7);
  CHECK(::atomic_fetch_add(&x, 1) == 12);
  CHECK(::atomic_fetch_add_explicit(&x, 1, memory_order_relaxed) == 13);
  CHECK(::atomic_fetch_sub(&x, 2) == 14);
  CHECK(::atomic_fetch_sub_explicit(&x, 2, memory_order_relaxed) == 12);
  CHECK(::atomic_fetch_or(&x, 0b0011) == 10);
  CHECK(::atomic_fetch_or_explicit(&x, 0b0100, memory_order_relaxed) == 11);
  CHECK(::atomic_fetch_and(&x, 0b0110) == 15);
  CHECK(::atomic_fetch_and_explicit(&x, 0b0010, memory_order_relaxed) == 6);
  CHECK(::atomic_fetch_xor(&x, 0b0011) == 2);
  CHECK(::atomic_fetch_xor_explicit(&x, 0b0001, memory_order_relaxed) == 1);
  CHECK(x.load() == 0);

  ::atomic_size_t* px = nullptr;
  (void)px;
  _Atomic(long*) ap(nullptr);
  long arr[3] = {};
  ::atomic_store(&ap, arr);
  CHECK(::atomic_fetch_add(&ap, 2) == arr);
  CHECK(ap.load() == arr + 2);

  CHECK(!::atomic_flag_test_and_set(&flag));
  CHECK(::atomic_flag_test_and_set_explicit(&flag, memory_order_acquire));
  ::atomic_flag_clear(&flag);
  CHECK(!flag.test());
  ::atomic_flag_test_and_set(&flag);
  ::atomic_flag_clear_explicit(&flag, memory_order_release);
  CHECK(!flag.test());

  ::atomic_thread_fence(memory_order_seq_cst);
  ::atomic_signal_fence(memory_order_acq_rel);
  counter.fetch_add(1);
  CHECK(::atomic_load(&counter) == 1);
  ::atomic_bool b(false);
  CHECK(!::atomic_exchange(&b, true));

  // the same objects are usable through both names
  std::atomic<int>& as_cxx = x;
  as_cxx.store(42);
  CHECK(::atomic_load(&x) == 42);
  return 0;
}
