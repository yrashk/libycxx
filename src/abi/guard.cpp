// libycxx ABI runtime: static-local guards ([ABI] 3.3.2).
// An archive member of their own: a ThreadSanitizer program gets the sanitizer runtime's
// __cxa_guard_* (Clang links that runtime whole, ahead of the program's archives), and this
// member is then never pulled in, instead of clashing with it (DECISIONS §6, sanitizer builds).
#include <cstdint>
#include <exception>
#include <ycxx/pal.h>

#include "entry.hpp"

namespace {

// The 64-bit guard object: its first byte is the "initialization complete" flag that compiled
// code tests inline. The second 32-bit word is ours: 0 idle, 1 initializing, 2 initializing with
// waiters.
struct guard_view {
  unsigned char* done;
  ycxx_pal_u32* state;
};
guard_view view(std::int64_t* __g) noexcept {
  return {reinterpret_cast<unsigned char*>(__g), reinterpret_cast<ycxx_pal_u32*>(__g) + 1};
}
enum : ycxx_pal_u32 { idle = 0, __busy = 1, busy_waiters = 2 };

// The guards this thread is initializing, innermost last. Re-entering one of them is recursive
// initialization, undefined by [stmt.dcl]/3; it would otherwise wait for itself forever, so it
// terminates. (Nesting deeper than the stack is not tracked.)
constexpr int max_nesting = 128;
constinit thread_local std::int64_t* initializing[max_nesting];
constinit thread_local int nesting;

bool initializing_here(std::int64_t* __g) noexcept {
  for (int i = 0; i < nesting && i < max_nesting; ++i)
    if (initializing[i] == __g)
      return true;
  return false;
}
void push_initializing(std::int64_t* __g) noexcept {
  if (nesting < max_nesting)
    initializing[nesting] = __g;
  ++nesting;
}
void pop_initializing() noexcept { --nesting; }

void end_guard(ycxx_pal_u32* state) noexcept {
  if (__atomic_exchange_n(state, idle, __ATOMIC_ACQ_REL) == busy_waiters)
    ycxx_pal_wake_all(state);
}

} // namespace

extern "C" {

// Returns 1 when the caller must run the initialization (then __cxa_guard_release or
// __cxa_guard_abort follows), 0 when it is already complete. A thread that finds another one
// initializing blocks until that completes or aborts ([stmt.dcl]/3).
int __ycxx_abi_guard_acquire(std::int64_t* __g) {
  const guard_view __v = view(__g);
  for (;;) {
    if (__atomic_load_n(__v.done, __ATOMIC_ACQUIRE))
      return 0;
    ycxx_pal_u32 s = idle;
    if (__atomic_compare_exchange_n(__v.state, &s, __busy, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)) {
      // Completed between the two loads?
      if (__atomic_load_n(__v.done, __ATOMIC_ACQUIRE)) {
        end_guard(__v.state);
        return 0;
      }
      push_initializing(__g);
      return 1;
    }
    if (initializing_here(__g))
      std::terminate();
    if (s == __busy && !__atomic_compare_exchange_n(__v.state, &s, busy_waiters, false, __ATOMIC_ACQUIRE,
                                                   __ATOMIC_ACQUIRE))
      continue;
    ycxx_pal_wait(__v.state, busy_waiters);
  }
}

void __ycxx_abi_guard_release(std::int64_t* __g) noexcept {
  const guard_view __v = view(__g);
  pop_initializing();
  __atomic_store_n(__v.done, 1, __ATOMIC_RELEASE);
  end_guard(__v.state);
}

void __ycxx_abi_guard_abort(std::int64_t* __g) noexcept {
  pop_initializing();
  end_guard(view(__g).state);
}

} // extern "C"
