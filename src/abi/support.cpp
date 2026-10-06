// libycxx ABI runtime: static-local guards ([ABI] 3.3.2), pure/deleted virtual calls,
// the helpers compilers call to throw (bad_cast, bad_typeid, bad_array_new_length),
// thread_local destructor registration, the new handler, and std::nothrow.
#include <cstdint>
#include <exception>
#include <new>
#include <typeinfo>
#include <ycxx/pal.h>

namespace {

// The 64-bit guard object: its first byte is the "initialization complete" flag that compiled
// code tests inline. The second 32-bit word is ours: 0 idle, 1 initializing, 2 initializing with
// waiters.
struct guard_view {
  unsigned char* done;
  __ycxx_pal_u32* state;
};
guard_view view(std::int64_t* __g) noexcept {
  return {reinterpret_cast<unsigned char*>(__g), reinterpret_cast<__ycxx_pal_u32*>(__g) + 1};
}
enum : __ycxx_pal_u32 { idle = 0, __busy = 1, busy_waiters = 2 };

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

void end_guard(__ycxx_pal_u32* state) noexcept {
  if (__atomic_exchange_n(state, idle, __ATOMIC_ACQ_REL) == busy_waiters)
    __ycxx_pal_wake_all(state);
}

std::new_handler new_handler_v;

[[noreturn]] void fatal(const char* __msg) noexcept { __ycxx_pal_abort(__msg); }

} // namespace

extern "C" {

// Returns 1 when the caller must run the initialization (then __cxa_guard_release or
// __cxa_guard_abort follows), 0 when it is already complete. A thread that finds another one
// initializing blocks until that completes or aborts ([stmt.dcl]/3).
[[__gnu__::__visibility__("hidden")]] int __cxa_guard_acquire(std::int64_t* __g) {
  const guard_view __v = view(__g);
  for (;;) {
    if (__atomic_load_n(__v.done, __ATOMIC_ACQUIRE))
      return 0;
    __ycxx_pal_u32 s = idle;
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
    __ycxx_pal_wait(__v.state, busy_waiters);
  }
}

[[__gnu__::__visibility__("hidden")]] void __cxa_guard_release(std::int64_t* __g) noexcept {
  const guard_view __v = view(__g);
  pop_initializing();
  __atomic_store_n(__v.done, 1, __ATOMIC_RELEASE);
  end_guard(__v.state);
}

[[__gnu__::__visibility__("hidden")]] void __cxa_guard_abort(std::int64_t* __g) noexcept {
  pop_initializing();
  end_guard(view(__g).state);
}

[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_pure_virtual() { fatal("pure virtual function called"); }
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_deleted_virtual() { fatal("deleted virtual function called"); }

[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_bad_cast() { throw std::bad_cast(); }
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_bad_typeid() { throw std::bad_typeid(); }
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_throw_bad_array_new_length() { throw std::bad_array_new_length(); }

[[__gnu__::__visibility__("hidden")]] int __cxa_thread_atexit(void (*dtor)(void*), void* __obj, void* __dso) noexcept {
  if (__ycxx_pal_thread_atexit(dtor, __obj, __dso) != 0)
    fatal("cannot register a thread_local destructor");
  return 0;
}

} // extern "C"

namespace [[__gnu__::__visibility__("hidden")]] std {

const nothrow_t nothrow{};

new_handler set_new_handler(new_handler __f) noexcept { return __atomic_exchange_n(&new_handler_v, __f, __ATOMIC_ACQ_REL); }
new_handler get_new_handler() noexcept { return __atomic_load_n(&new_handler_v, __ATOMIC_ACQUIRE); }

} // namespace std
