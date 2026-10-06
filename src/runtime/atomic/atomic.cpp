// libycxx runtime (hosted and freestanding archives): the lock table behind lock-based atomic
// objects and the slot table behind atomic waiting and notifying (ycxx/core/atomic_base.hpp).
//
// Both tables are keyed by the object's address and blocked on through the PAL's address wait
// (ycxx_pal_wait / ycxx_pal_wake_*); the freestanding archive's default PAL hooks return at
// once, which turns every wait into a spin.
#include <ycxx/core/atomic_base.hpp>
#include <ycxx/pal.h>
#include "wait_table.hpp"

namespace {

constexpr unsigned table_size = 256;

unsigned slot_of(const volatile void* __addr) noexcept {
  // Fibonacci hashing of the address; the low bits of an aligned address carry little.
  const auto a = reinterpret_cast<__UINTPTR_TYPE__>(__addr);
  return static_cast<unsigned>((static_cast<unsigned long long>(a >> 2) * 0x9e3779b97f4a7c15ull) >> 56) %
         table_size;
}

// One cache line per entry, so unrelated objects do not contend through false sharing.
struct alignas(64) lock_entry {
  ycxx_pal_u32 state; // 0: free, 1: held, 2: held and possibly waited for
};
constinit lock_entry locks[table_size] = {};

struct alignas(64) wait_entry {
  ycxx_pal_u32 version; // bumped by every notification with waiters
  ycxx_pal_u32 __waiters; // threads between atomic_wait_prepare and the end of their wait
};
constinit wait_entry waits[table_size] = {};

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

void __atomic_lock(const volatile void* __addr) noexcept {
  ycxx_pal_u32* s = &locks[slot_of(__addr)].state;
  ycxx_pal_u32 c = 0;
  if (__atomic_compare_exchange_n(s, &c, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
    return;
  for (int i = 0; i < 64; ++i) {
    c = 0;
    if (__atomic_load_n(s, __ATOMIC_RELAXED) == 0 &&
        __atomic_compare_exchange_n(s, &c, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
      return;
  }
  while (__atomic_exchange_n(s, 2, __ATOMIC_ACQUIRE) != 0)
    ::ycxx_pal_wait(s, 2);
}

void __atomic_unlock(const volatile void* __addr) noexcept {
  ycxx_pal_u32* s = &locks[slot_of(__addr)].state;
  if (__atomic_exchange_n(s, 0, __ATOMIC_RELEASE) == 2)
    ::ycxx_pal_wake_one(s);
}

// The protocol: a waiter increments `__waiters`, reads `version`, then (after a seq_cst fence)
// re-checks the waited-for value and blocks while `version` is unchanged. A notifier has stored
// the new value before it calls atomic_notify, which issues a seq_cst fence and reads `__waiters`.
// By the two fences, either the notifier sees the waiter registered (and bumps `version` and
// wakes the slot, after which the waiter's futex wait cannot sleep on the old version), or the
// waiter's re-check sees the new value.
std::uint32_t __atomic_wait_prepare(const volatile void* __addr) noexcept {
  wait_entry& e = waits[slot_of(__addr)];
  __atomic_fetch_add(&e.__waiters, 1, __ATOMIC_SEQ_CST);
  const ycxx_pal_u32 __ticket = __atomic_load_n(&e.version, __ATOMIC_ACQUIRE);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  return __ticket;
}

void __atomic_wait_block(const volatile void* __addr, std::uint32_t __ticket) noexcept {
  wait_entry& e = waits[slot_of(__addr)];
  ::ycxx_pal_wait(&e.version, __ticket);
  __atomic_fetch_sub(&e.__waiters, 1, __ATOMIC_RELAXED);
}

void __atomic_wait_cancel(const volatile void* __addr) noexcept {
  __atomic_fetch_sub(&waits[slot_of(__addr)].__waiters, 1, __ATOMIC_RELAXED);
}

void __atomic_notify(const volatile void* __addr) noexcept {
  wait_entry& e = waits[slot_of(__addr)];
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  if (__atomic_load_n(&e.__waiters, __ATOMIC_RELAXED) == 0)
    return;
  __atomic_fetch_add(&e.version, 1, __ATOMIC_RELEASE);
  ::ycxx_pal_wake_all(&e.version);
}

// For the hosted timed wait (src/hosted/thread.cpp), which shares the slot table.
std::uint32_t* __atomic_wait_entry(const volatile void* __addr, std::uint32_t*& __waiters) noexcept {
  wait_entry& e = waits[slot_of(__addr)];
  __waiters = &e.__waiters;
  return &e.version;
}

}} // namespace __ycxx::__detail
