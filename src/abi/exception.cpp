// libycxx ABI runtime: throwing and catching ([ABI-EH] 2.4-2.5), the per-thread exception
// state, terminate, and the exception-object lifetime shared with exception_ptr.
#include "eh.hpp"
#include "entry.hpp"
#include "internal.hpp"

#include <new>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __abi {
namespace {

// ---- Layout ----
// Thrown objects are aligned for any type a compiler may throw without extra care: GCC stores
// an over-aligned exception with aligned vector moves right after __cxa_allocate_exception
// (alignas(64) types included), so the object is placed on a 64-byte boundary. Each block is
// 64-byte aligned and starts with `header_pad` unused bytes, so that the header ends there.
constexpr std::size_t object_alignment = 64;
constexpr std::size_t header_pad =
    (sizeof(__exception_header) + object_alignment - 1) / object_alignment * object_alignment -
    sizeof(__exception_header);

// ---- Emergency pool ----
// When the heap is exhausted, exceptions (typically bad_alloc) come from a fixed pool of
// 1 KiB blocks ([ABI-EH] 3.4.1), claimed with an atomic bitmap. An allocation that cannot be
// served at all terminates (2.4.2).
constexpr std::size_t pool_block = 1024;
constexpr std::size_t pool_blocks = 64;
alignas(object_alignment) unsigned char pool[pool_blocks * pool_block];
unsigned long long pool_used; // bit i: block i in use

void* pool_allocate(std::size_t size) noexcept {
  if (size > pool_block)
    return nullptr;
  unsigned long long __y_used = __atomic_load_n(&pool_used, __ATOMIC_RELAXED);
  while (~__y_used != 0) {
    const int i = __builtin_ctzll(~__y_used);
    if (__atomic_compare_exchange_n(&pool_used, &__y_used, __y_used | (1ULL << i), false, __ATOMIC_ACQUIRE,
                                    __ATOMIC_RELAXED))
      return pool + i * pool_block;
  }
  return nullptr;
}
bool pool_free(void* p) noexcept {
  unsigned char* b = static_cast<unsigned char*>(p);
  if (b < pool || b >= pool + sizeof(pool))
    return false;
  const std::size_t i = static_cast<std::size_t>(b - pool) / pool_block;
  __atomic_fetch_and(&pool_used, ~(1ULL << i), __ATOMIC_RELEASE);
  return true;
}

// A header followed by `__thrown_size` bytes, the header zero-initialized (so unset cached fields
// read as null).
__exception_header* allocate_header(std::size_t __thrown_size) noexcept {
  const std::size_t size = header_pad + sizeof(__exception_header) + __thrown_size;
  void* p = ycxx_pal_allocate(size, object_alignment);
  std::size_t recorded = size;
  if (!p) {
    p = pool_allocate(size);
    recorded = 0;
  }
  if (!p)
    ycxx_pal_abort("cannot allocate an exception object");
  __exception_header* h = reinterpret_cast<__exception_header*>(static_cast<unsigned char*>(p) + header_pad);
  __builtin_memset(h, 0, sizeof(__exception_header));
  h->__allocation_size = recorded;
  return h;
}
void free_header(__exception_header* h) noexcept {
  unsigned char* block = reinterpret_cast<unsigned char*>(h) - header_pad;
  if (h->__allocation_size == 0)
    pool_free(block);
  else
    ycxx_pal_deallocate(block, h->__allocation_size, object_alignment);
}

// Called when something other than this runtime deletes one of our exceptions
// (_Unwind_DeleteException from a foreign catch), or when a forced unwind ends.
void cleanup_native(_Unwind_Reason_Code reason, _Unwind_Exception* __ue) {
  if (reason != _URC_FOREIGN_EXCEPTION_CAUGHT && reason != _URC_NO_REASON)
    std::terminate();
  // Caught (and finished) by another runtime's handler: no longer uncaught.
  if (reason == _URC_FOREIGN_EXCEPTION_CAUGHT)
    --__header_of_unwind(__ue)->__counted_in->uncaught_exceptions;
  release(__header_of_unwind(__ue));
}


constinit thread_local __eh_globals thread_globals{};

void write_err(const char* s) noexcept {
  ycxx_pal_size n = 0, __w = 0;
  while (s[n])
    ++n;
  ycxx_pal_write(ycxx_pal_stderr, s, n, &__w);
}

// The default terminate handler: reports the current exception (its mangled type name, and
// what() for a std::exception) and aborts.
constinit thread_local bool in_default_terminate = false;

void default_terminate() {
  // what() may itself end in terminate; report once.
  if (in_default_terminate)
    ycxx_pal_abort("terminate called recursively");
  in_default_terminate = true;
  __eh_globals* __g = __globals();
  __exception_header* h = __g->__caught_exceptions;
  if (h && __is_native(h->__unwind_header.exception_class)) {
    write_err("terminate called after throwing an exception of type ");
    write_err(h->__exception_type->name());
    void* __obj = object_of(h);
    if (__catch_matches(&typeid(std::exception), h->__exception_type, &__obj)) {
      write_err(": ");
      write_err(static_cast<const std::exception*>(__obj)->what());
    }
    write_err("\n");
    ycxx_pal_abort(nullptr);
  }
  if (h)
    ycxx_pal_abort("terminate called after throwing a foreign exception");
  ycxx_pal_abort("terminate called without an active exception");
}

// The current handler; starts as (and a null argument to set_terminate restores) the default.
std::terminate_handler terminate_handler_v = default_terminate;

[[noreturn]] void call_terminate_handler(std::terminate_handler __f) noexcept {
  // [terminate.handler]/2: a terminate handler shall not return; one that does (or throws)
  // ends the program here.
  try {
    __f();
  } catch (...) {
  }
  ycxx_pal_abort("terminate handler returned");
}

} // namespace

__eh_globals* __globals() noexcept { return &thread_globals; }

__exception_header* __retain_primary(__exception_header* h) noexcept {
  __exception_header* p = __is_dependent(h) ? __header_of_object(h->__primary_object) : h;
  __atomic_fetch_add(&p->__reference_count, 1, __ATOMIC_RELAXED);
  return p;
}

void release(__exception_header* h) noexcept {
  if (__is_dependent(h)) {
    __exception_header* p = __header_of_object(h->__primary_object);
    free_header(h);
    h = p;
  }
  if (__atomic_sub_fetch(&h->__reference_count, 1, __ATOMIC_ACQ_REL) == 0) {
    if (h->__exception_destructor)
      h->__exception_destructor(h + 1);
    free_header(h);
  }
}

// release() for the end of a handler ([except.throw]/4.1): the exception object's destructor may
// exit via an exception, which then propagates from the end of the handler (/9 and
// [except.terminate]/1 call terminate only during unwinding); the header is freed either way.
void release_at_handler_exit(__exception_header* h) {
  if (__is_dependent(h)) {
    __exception_header* p = __header_of_object(h->__primary_object);
    free_header(h);
    h = p;
  }
  if (__atomic_sub_fetch(&h->__reference_count, 1, __ATOMIC_ACQ_REL) == 0) {
    struct free_on_exit {
      __exception_header* h;
      ~free_on_exit() { free_header(h); }
    } __guard{h};
    if (h->__exception_destructor)
      h->__exception_destructor(h + 1);
  }
}

[[noreturn]] void __terminate_for(_Unwind_Exception* __ue) noexcept {
  __ycxx_abi_begin_catch(__ue);
  std::terminate();
}

// Throws an exception whose header is already filled in apart from the unwind fields. Inlined
// into its callers: each frame between the throw and the handler is unwound twice (search and
// cleanup phases), so a helper frame of its own would make every throw slower.
[[noreturn]] [[__gnu__::__always_inline__]] inline void raise(__exception_header* h) {
  h->unexpected_handler = nullptr;
  h->terminate_handler = std::get_terminate();
  h->__unwind_header.exception_cleanup = cleanup_native;
  h->__counted_in = __globals();
  ++h->__counted_in->uncaught_exceptions;
  _Unwind_RaiseException(&h->__unwind_header);
  // No handler: [except.handle]/9.
  __terminate_for(&h->__unwind_header);
}

// rethrow_exception: a dependent exception referring to `primary` (which gains a reference).
[[noreturn]] void __rethrow_primary(void* __primary_object) {
  __exception_header* p = __header_of_object(__primary_object);
  __exception_header* d = allocate_header(0);
  d->__primary_object = __primary_object;
  d->__exception_type = p->__exception_type;
  d->__exception_destructor = p->__exception_destructor;
  d->__unwind_header.exception_class = __dependent_class;
  __retain_primary(p);
  raise(d);
}

}} // namespace __ycxx::__abi

using namespace __ycxx::__abi;

// The entry points under the runtime's own names (entry.hpp, DECISIONS §20.6); the ABI's names
// (__cxa_throw, ...) are src/abi/entry/cxa_exception.cpp's forwarders.

extern "C" {

void* __ycxx_abi_get_globals() noexcept { return __globals(); }
void* __ycxx_abi_get_globals_fast() noexcept { return __globals(); }

void* __ycxx_abi_allocate_exception(std::size_t __thrown_size) noexcept {
  __exception_header* h = allocate_header(__thrown_size);
  h->__reference_count = 1;
  return h + 1;
}

void __ycxx_abi_free_exception(void* __thrown) noexcept { free_header(__header_of_object(__thrown)); }

// (GCC predeclares __cxa_throw with a void* type_info parameter.)
void __ycxx_abi_throw(void* __thrown, void* __tinfo, void (*__dest)(void*)) {
  __exception_header* h = __header_of_object(__thrown);
  h->__exception_type = static_cast<std::type_info*>(__tinfo);
  h->__exception_destructor = __dest;
  h->__unwind_header.exception_class = __primary_class;
  raise(h);
}

void* __ycxx_abi_get_exception_ptr(void* __ue) noexcept {
  _Unwind_Exception* __u = static_cast<_Unwind_Exception*>(__ue);
  if (!__is_native(__u->exception_class))
    return __u + 1;
  return __header_of_unwind(__u)->__adjusted_ptr;
}

void* __ycxx_abi_begin_catch(void* __ue) noexcept {
  _Unwind_Exception* __u = static_cast<_Unwind_Exception*>(__ue);
  __eh_globals* __g = __globals();
  __exception_header* h = __header_of_unwind(__u);
  if (!__is_native(__u->exception_class)) {
    // A foreign exception has no header of ours; it can only be the bottom of the caught stack
    // (nothing can be linked below it), so it may not be caught while another is.
    if (__g->__caught_exceptions)
      std::terminate();
    __g->__caught_exceptions = h;
    return __u + 1;
  }
  // handler_count is negative while the exception is being rethrown.
  const int count = h->__handler_count < 0 ? -h->__handler_count + 1 : h->__handler_count + 1;
  h->__handler_count = count;
  if (h != __g->__caught_exceptions) {
    h->__next_exception = __g->__caught_exceptions;
    __g->__caught_exceptions = h;
  }
  --h->__counted_in->uncaught_exceptions; // the throwing image's count (eh.hpp)
  return h->__adjusted_ptr;
}

void __ycxx_abi_end_catch() {
  __eh_globals* __g = __globals();
  __exception_header* h = __g->__caught_exceptions;
  if (!h)
    return;
  if (!__is_native(h->__unwind_header.exception_class)) {
    __g->__caught_exceptions = nullptr;
    _Unwind_DeleteException(&h->__unwind_header);
    return;
  }
  if (h->__handler_count < 0) {
    // Rethrown: still in flight; leaves the caught stack when its last handler exits.
    if (++h->__handler_count == 0)
      __g->__caught_exceptions = h->__next_exception;
  } else if (--h->__handler_count == 0) {
    __g->__caught_exceptions = h->__next_exception;
    release_at_handler_exit(h);
  }
}

void __ycxx_abi_rethrow() {
  __eh_globals* __g = __globals();
  __exception_header* h = __g->__caught_exceptions;
  if (!h)
    std::terminate();
  if (__is_native(h->__unwind_header.exception_class)) {
    // Already being rethrown (a `throw;` while unwinding from an earlier one, e.g. in a
    // destructor's handler): its unwind header is in use, so throw a dependent exception that
    // refers to the same object.
    if (h->__handler_count < 0)
      __rethrow_primary(object_of(h));
    h->__handler_count = -h->__handler_count;
    h->__counted_in = __g;
    ++__g->uncaught_exceptions;
  } else {
    // Ending the foreign handler must not delete it: it is in flight again.
    __g->__caught_exceptions = nullptr;
  }
  _Unwind_Resume_or_Rethrow(&h->__unwind_header);
  __terminate_for(&h->__unwind_header);
}

std::type_info* __ycxx_abi_current_exception_type() noexcept {
  __exception_header* h = __globals()->__caught_exceptions;
  if (!h || !__is_native(h->__unwind_header.exception_class))
    return nullptr;
  return h->__exception_type;
}

// Landing pads that must terminate: GCC calls this for a violated exception specification
// (C++26 has only noexcept, which GCC encodes as a gap in the call-site table instead).
[[noreturn]] void __ycxx_abi_call_unexpected(void* __ue) noexcept { __terminate_for(static_cast<_Unwind_Exception*>(__ue)); }
[[noreturn]] void __ycxx_abi_call_terminate(void* __ue) noexcept {
  if (__ue)
    __ycxx_abi_begin_catch(__ue);
  __ycxx_abi_terminate();
}

// std::terminate ([except.terminate]); std::terminate itself is src/abi/entry/cxa_terminate.cpp's.
[[noreturn]] void __ycxx_abi_terminate() noexcept {
  __ycxx::__abi::call_terminate_handler(std::get_terminate());
}

} // extern "C"

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [set.terminate]/2 leaves open whether null designates the default handler; here it does.
terminate_handler set_terminate(terminate_handler __f) noexcept {
  return __atomic_exchange_n(&__ycxx::__abi::terminate_handler_v, __f ? __f : __ycxx::__abi::default_terminate,
                             __ATOMIC_ACQ_REL);
}
terminate_handler get_terminate() noexcept {
  return __atomic_load_n(&__ycxx::__abi::terminate_handler_v, __ATOMIC_ACQUIRE);
}

int uncaught_exceptions() noexcept { return static_cast<int>(__ycxx::__abi::__globals()->uncaught_exceptions); }

}} // namespace std
