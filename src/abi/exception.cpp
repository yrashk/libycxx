// libycxx ABI runtime: throwing and catching ([ABI-EH] 2.4-2.5), the per-thread exception
// state, terminate, and the exception-object lifetime shared with exception_ptr.
#include "eh.hpp"
#include "internal.hpp"

#include <new>
#include <ycxx/pal.h>

namespace ycxx::abi {
namespace {

// ---- Emergency pool ----
// When the heap is exhausted, exceptions (typically bad_alloc) come from a fixed pool of
// blocks, claimed with an atomic bitmap. [ABI-EH] 3.4.1 allows this; an allocation that cannot
// be served at all terminates (2.4.2).
constexpr std::size_t pool_block = 512;
constexpr std::size_t pool_blocks = 64;
alignas(alignof(exception_header)) unsigned char pool[pool_blocks * pool_block];
unsigned long long pool_used; // bit i: block i in use

void* pool_allocate(std::size_t size) noexcept {
  if (size > pool_block)
    return nullptr;
  unsigned long long used = __atomic_load_n(&pool_used, __ATOMIC_RELAXED);
  while (~used != 0) {
    const int i = __builtin_ctzll(~used);
    if (__atomic_compare_exchange_n(&pool_used, &used, used | (1ULL << i), false, __ATOMIC_ACQUIRE,
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

// A header followed by `thrown_size` bytes, zero-initialized like [ABI-EH] implementations
// commonly do (so unset cached fields read as null).
exception_header* allocate_header(std::size_t thrown_size) noexcept {
  const std::size_t size = sizeof(exception_header) + thrown_size;
  void* p = ycxx_pal_allocate(size, alignof(exception_header));
  std::size_t recorded = size;
  if (!p) {
    p = pool_allocate(size);
    recorded = 0;
  }
  if (!p)
    std::terminate();
  __builtin_memset(p, 0, sizeof(exception_header));
  exception_header* h = static_cast<exception_header*>(p);
  h->allocation_size = recorded;
  return h;
}
void free_header(exception_header* h) noexcept {
  if (h->allocation_size == 0)
    pool_free(h);
  else
    ycxx_pal_deallocate(h, h->allocation_size, alignof(exception_header));
}

// Called when something other than this runtime deletes one of our exceptions
// (_Unwind_DeleteException from a foreign catch), or when a forced unwind ends.
void cleanup_native(_Unwind_Reason_Code reason, _Unwind_Exception* ue) {
  if (reason != _URC_FOREIGN_EXCEPTION_CAUGHT && reason != _URC_NO_REASON)
    std::terminate();
  release(header_of_unwind(ue));
}

std::terminate_handler terminate_handler_v;

constinit thread_local eh_globals thread_globals{};

void write_err(const char* s) noexcept {
  ycxx_pal_size n = 0, w = 0;
  while (s[n])
    ++n;
  ycxx_pal_write(ycxx_pal_stderr, s, n, &w);
}

// The default terminate handler: reports the current exception (its mangled type name, and
// what() for a std::exception) and aborts.
void default_terminate() {
  eh_globals* g = globals();
  exception_header* h = g->caught_exceptions;
  if (h && is_native(h->unwind_header.exception_class)) {
    write_err("terminate called after throwing an exception of type ");
    write_err(h->exception_type->name());
    void* obj = object_of(h);
    if (catch_matches(&typeid(std::exception), h->exception_type, &obj)) {
      write_err(": ");
      write_err(static_cast<const std::exception*>(obj)->what());
    }
    write_err("\n");
    ycxx_pal_abort("terminate called after throwing an exception");
  }
  if (h)
    ycxx_pal_abort("terminate called after throwing a foreign exception");
  ycxx_pal_abort("terminate called without an active exception");
}

[[noreturn]] void call_terminate_handler(std::terminate_handler f) noexcept {
  // [terminate.handler]/2: a terminate handler shall not return; one that does (or throws)
  // ends the program here.
  try {
    f();
  } catch (...) {
  }
  ycxx_pal_abort("terminate handler returned");
}

} // namespace

eh_globals* globals() noexcept { return &thread_globals; }

exception_header* retain_primary(exception_header* h) noexcept {
  exception_header* p = is_dependent(h) ? header_of_object(h->primary_object) : h;
  __atomic_fetch_add(&p->reference_count, 1, __ATOMIC_RELAXED);
  return p;
}

void release(exception_header* h) noexcept {
  if (is_dependent(h)) {
    exception_header* p = header_of_object(h->primary_object);
    free_header(h);
    h = p;
  }
  if (__atomic_sub_fetch(&h->reference_count, 1, __ATOMIC_ACQ_REL) == 0) {
    if (h->exception_destructor)
      h->exception_destructor(h + 1);
    free_header(h);
  }
}

[[noreturn]] void terminate_for(_Unwind_Exception* ue) noexcept {
  __cxa_begin_catch(ue);
  std::terminate();
}

// Throws an exception whose header is already filled in apart from the unwind fields.
[[noreturn]] void raise(exception_header* h) {
  h->unexpected_handler = nullptr;
  h->terminate_handler = std::get_terminate();
  h->unwind_header.exception_cleanup = cleanup_native;
  ++globals()->uncaught_exceptions;
  _Unwind_RaiseException(&h->unwind_header);
  // No handler: [except.handle]/9.
  terminate_for(&h->unwind_header);
}

// rethrow_exception: a dependent exception referring to `primary` (which gains a reference).
[[noreturn]] void rethrow_primary(void* primary_object) {
  exception_header* p = header_of_object(primary_object);
  exception_header* d = allocate_header(0);
  d->primary_object = primary_object;
  d->exception_type = p->exception_type;
  d->exception_destructor = p->exception_destructor;
  d->unwind_header.exception_class = dependent_class;
  retain_primary(p);
  raise(d);
}

} // namespace ycxx::abi

using namespace ycxx::abi;

extern "C" {

eh_globals* __cxa_get_globals() noexcept { return globals(); }
eh_globals* __cxa_get_globals_fast() noexcept { return globals(); }

void* __cxa_allocate_exception(std::size_t thrown_size) noexcept {
  exception_header* h = allocate_header(thrown_size);
  h->reference_count = 1;
  return h + 1;
}

void __cxa_free_exception(void* thrown) noexcept { free_header(header_of_object(thrown)); }

// (GCC predeclares __cxa_throw with a void* type_info parameter.)
[[noreturn]] void __cxa_throw(void* thrown, void* tinfo, void (*dest)(void*)) {
  exception_header* h = header_of_object(thrown);
  h->exception_type = static_cast<std::type_info*>(tinfo);
  h->exception_destructor = dest;
  h->unwind_header.exception_class = primary_class;
  raise(h);
}

void* __cxa_get_exception_ptr(void* ue) noexcept {
  _Unwind_Exception* u = static_cast<_Unwind_Exception*>(ue);
  if (!is_native(u->exception_class))
    return u + 1;
  return header_of_unwind(u)->adjusted_ptr;
}

void* __cxa_begin_catch(void* ue) noexcept {
  _Unwind_Exception* u = static_cast<_Unwind_Exception*>(ue);
  eh_globals* g = globals();
  exception_header* h = header_of_unwind(u);
  if (!is_native(u->exception_class)) {
    // A foreign exception has no header of ours; it can only be the bottom of the caught stack
    // (nothing can be linked below it), so it may not be caught while another is.
    if (g->caught_exceptions)
      std::terminate();
    g->caught_exceptions = h;
    return u + 1;
  }
  // handler_count is negative while the exception is being rethrown.
  const int count = h->handler_count < 0 ? -h->handler_count + 1 : h->handler_count + 1;
  h->handler_count = count;
  if (h != g->caught_exceptions) {
    h->next_exception = g->caught_exceptions;
    g->caught_exceptions = h;
  }
  --g->uncaught_exceptions;
  return h->adjusted_ptr;
}

void __cxa_end_catch() {
  eh_globals* g = globals();
  exception_header* h = g->caught_exceptions;
  if (!h)
    return;
  if (!is_native(h->unwind_header.exception_class)) {
    g->caught_exceptions = nullptr;
    _Unwind_DeleteException(&h->unwind_header);
    return;
  }
  if (h->handler_count < 0) {
    // Rethrown: still in flight; leaves the caught stack when its last handler exits.
    if (++h->handler_count == 0)
      g->caught_exceptions = h->next_exception;
  } else if (--h->handler_count == 0) {
    g->caught_exceptions = h->next_exception;
    release(h);
  }
}

[[noreturn]] void __cxa_rethrow() {
  eh_globals* g = globals();
  exception_header* h = g->caught_exceptions;
  if (!h)
    std::terminate();
  if (is_native(h->unwind_header.exception_class)) {
    h->handler_count = -h->handler_count;
    ++g->uncaught_exceptions;
  } else {
    // Ending the foreign handler must not delete it: it is in flight again.
    g->caught_exceptions = nullptr;
  }
  _Unwind_Resume_or_Rethrow(&h->unwind_header);
  terminate_for(&h->unwind_header);
}

std::type_info* __cxa_current_exception_type() noexcept {
  exception_header* h = globals()->caught_exceptions;
  if (!h || !is_native(h->unwind_header.exception_class))
    return nullptr;
  return h->exception_type;
}

// Landing pads that must terminate: GCC calls this for a violated exception specification
// (C++26 has only noexcept, which GCC encodes as a gap in the call-site table instead).
[[noreturn]] void __cxa_call_unexpected(void* ue) noexcept { terminate_for(static_cast<_Unwind_Exception*>(ue)); }
[[noreturn]] void __cxa_call_terminate(void* ue) noexcept {
  if (ue)
    __cxa_begin_catch(ue);
  std::terminate();
}

} // extern "C"

namespace std {

terminate_handler set_terminate(terminate_handler f) noexcept {
  return __atomic_exchange_n(&ycxx::abi::terminate_handler_v, f, __ATOMIC_ACQ_REL);
}
terminate_handler get_terminate() noexcept {
  terminate_handler f = __atomic_load_n(&ycxx::abi::terminate_handler_v, __ATOMIC_ACQUIRE);
  return f ? f : ycxx::abi::default_terminate;
}
[[noreturn]] void terminate() noexcept { ycxx::abi::call_terminate_handler(get_terminate()); }

int uncaught_exceptions() noexcept { return static_cast<int>(ycxx::abi::globals()->uncaught_exceptions); }

} // namespace std
