// libycxx ABI runtime: static-local guards ([ABI] 3.3.2), pure/deleted virtual calls,
// the helpers compilers call to throw (bad_cast, bad_typeid, bad_array_new_length),
// thread_local destructor registration, the new handler, and std::nothrow.
#include <cstdint>
#include <new>
#include <typeinfo>
#include <ycxx/pal.h>

namespace {

// The 64-bit guard object: its first byte is the "initialization complete" flag that compiled
// code tests inline. The second 32-bit word is ours: 0 idle, 1 initializing, 2 initializing with
// waiters.
struct guard_view {
  unsigned char* done;
  ycxx_pal_u32* state;
};
guard_view view(std::int64_t* g) noexcept {
  return {reinterpret_cast<unsigned char*>(g), reinterpret_cast<ycxx_pal_u32*>(g) + 1};
}
enum : ycxx_pal_u32 { idle = 0, busy = 1, busy_waiters = 2 };

void end_guard(ycxx_pal_u32* state) noexcept {
  if (__atomic_exchange_n(state, idle, __ATOMIC_ACQ_REL) == busy_waiters)
    ycxx_pal_wake_all(state);
}

std::new_handler new_handler_v;

[[noreturn]] void fatal(const char* msg) noexcept { ycxx_pal_abort(msg); }

} // namespace

extern "C" {

// Returns 1 when the caller must run the initialization (then __cxa_guard_release or
// __cxa_guard_abort follows), 0 when it is already complete. A thread that finds another one
// initializing blocks until that completes or aborts ([stmt.dcl]/3).
int __cxa_guard_acquire(std::int64_t* g) {
  const guard_view v = view(g);
  for (;;) {
    if (__atomic_load_n(v.done, __ATOMIC_ACQUIRE))
      return 0;
    ycxx_pal_u32 s = idle;
    if (__atomic_compare_exchange_n(v.state, &s, busy, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)) {
      // Completed between the two loads?
      if (__atomic_load_n(v.done, __ATOMIC_ACQUIRE)) {
        end_guard(v.state);
        return 0;
      }
      return 1;
    }
    if (s == busy && !__atomic_compare_exchange_n(v.state, &s, busy_waiters, false, __ATOMIC_ACQUIRE,
                                                   __ATOMIC_ACQUIRE))
      continue;
    ycxx_pal_wait(v.state, busy_waiters);
  }
}

void __cxa_guard_release(std::int64_t* g) noexcept {
  const guard_view v = view(g);
  __atomic_store_n(v.done, 1, __ATOMIC_RELEASE);
  end_guard(v.state);
}

void __cxa_guard_abort(std::int64_t* g) noexcept { end_guard(view(g).state); }

[[noreturn]] void __cxa_pure_virtual() { fatal("pure virtual function called"); }
[[noreturn]] void __cxa_deleted_virtual() { fatal("deleted virtual function called"); }

[[noreturn]] void __cxa_bad_cast() { throw std::bad_cast(); }
[[noreturn]] void __cxa_bad_typeid() { throw std::bad_typeid(); }
[[noreturn]] void __cxa_throw_bad_array_new_length() { throw std::bad_array_new_length(); }

int __cxa_thread_atexit(void (*dtor)(void*), void* obj, void* dso) noexcept {
  if (ycxx_pal_thread_atexit(dtor, obj, dso) != 0)
    fatal("cannot register a thread_local destructor");
  return 0;
}

} // extern "C"

namespace std {

const nothrow_t nothrow{};

new_handler set_new_handler(new_handler f) noexcept { return __atomic_exchange_n(&new_handler_v, f, __ATOMIC_ACQ_REL); }
new_handler get_new_handler() noexcept { return __atomic_load_n(&new_handler_v, __ATOMIC_ACQUIRE); }

} // namespace std
