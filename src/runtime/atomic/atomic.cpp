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

unsigned slot_of(const volatile void* addr) noexcept {
  // Fibonacci hashing of the address; the low bits of an aligned address carry little.
  const auto a = reinterpret_cast<__UINTPTR_TYPE__>(addr);
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
  ycxx_pal_u32 waiters; // threads between atomic_wait_prepare and the end of their wait
};
constinit wait_entry waits[table_size] = {};

} // namespace

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

void atomic_lock(const volatile void* addr) noexcept {
  ycxx_pal_u32* s = &locks[slot_of(addr)].state;
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

void atomic_unlock(const volatile void* addr) noexcept {
  ycxx_pal_u32* s = &locks[slot_of(addr)].state;
  if (__atomic_exchange_n(s, 0, __ATOMIC_RELEASE) == 2)
    ::ycxx_pal_wake_one(s);
}

// The protocol: a waiter increments `waiters`, reads `version`, then (after a seq_cst fence)
// re-checks the waited-for value and blocks while `version` is unchanged. A notifier has stored
// the new value before it calls atomic_notify, which issues a seq_cst fence and reads `waiters`.
// By the two fences, either the notifier sees the waiter registered (and bumps `version` and
// wakes the slot, after which the waiter's futex wait cannot sleep on the old version), or the
// waiter's re-check sees the new value.
std::uint32_t atomic_wait_prepare(const volatile void* addr) noexcept {
  wait_entry& e = waits[slot_of(addr)];
  __atomic_fetch_add(&e.waiters, 1, __ATOMIC_SEQ_CST);
  const ycxx_pal_u32 ticket = __atomic_load_n(&e.version, __ATOMIC_ACQUIRE);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  return ticket;
}

void atomic_wait_block(const volatile void* addr, std::uint32_t ticket) noexcept {
  wait_entry& e = waits[slot_of(addr)];
  ::ycxx_pal_wait(&e.version, ticket);
  __atomic_fetch_sub(&e.waiters, 1, __ATOMIC_RELAXED);
}

void atomic_wait_cancel(const volatile void* addr) noexcept {
  __atomic_fetch_sub(&waits[slot_of(addr)].waiters, 1, __ATOMIC_RELAXED);
}

void atomic_notify(const volatile void* addr) noexcept {
  wait_entry& e = waits[slot_of(addr)];
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  if (__atomic_load_n(&e.waiters, __ATOMIC_RELAXED) == 0)
    return;
  __atomic_fetch_add(&e.version, 1, __ATOMIC_RELEASE);
  ::ycxx_pal_wake_all(&e.version);
}

// For the hosted timed wait (src/hosted/thread.cpp), which shares the slot table.
std::uint32_t* atomic_wait_entry(const volatile void* addr, std::uint32_t*& waiters) noexcept {
  wait_entry& e = waits[slot_of(addr)];
  waiters = &e.waiters;
  return &e.version;
}

}} // namespace ycxx::detail
