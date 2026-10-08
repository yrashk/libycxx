// libycxx ABI runtime: the exception_ptr operations (include/ycxx/core/exception_ptr.hpp).
#include "eh.hpp"
#include "internal.hpp"

#include <exception>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __abi {

void __exception_ptr_retain(void* __object) noexcept { __retain_primary(__header_of_object(__object)); }

void __exception_ptr_release(void* __object) noexcept { release(__header_of_object(__object)); }

void* __current_exception_object() noexcept {
  __exception_header* h = __globals()->__caught_exceptions;
  if (!h)
    return nullptr;
  if (!__is_native(h->__unwind_header.exception_class)) {
    // A foreign exception cannot be referred to: [propagation]/9 lets the result refer to a
    // bad_exception instead.
    void* __obj = __ycxx_abi_allocate_exception(sizeof(std::bad_exception));
    ::new (__obj) std::bad_exception();
    __exception_header* b = __header_of_object(__obj);
    b->__exception_type = const_cast<std::type_info*>(&typeid(std::bad_exception));
    b->__exception_destructor = [](void* p) { static_cast<std::bad_exception*>(p)->~bad_exception(); };
    b->__unwind_header.exception_class = __primary_class;
    return __obj;
  }
  __exception_header* p = __retain_primary(h);
  return p + 1;
}

[[noreturn]] void __rethrow_exception_object(void* __object) { __rethrow_primary(__object); }

// make_exception_ptr without exceptions: a primary exception that is never thrown. Its header
// is what __cxa_throw would record; the unwind fields are set if it is ever rethrown (as a
// dependent exception, rethrow_primary).
void* __exception_object_create(std::size_t size, const std::type_info* type, void (*destroy)(void*)) noexcept {
  void* __obj = __ycxx_abi_allocate_exception(size);
  __exception_header* h = __header_of_object(__obj);
  h->__exception_type = const_cast<std::type_info*>(type);
  h->__exception_destructor = destroy;
  h->__unwind_header.exception_class = __primary_class;
  return __obj;
}

const void* __exception_object_as(void* __object, const std::type_info& __handler) noexcept {
  __exception_header* h = __header_of_object(__object);
  void* adjusted = __object;
  if (!__catch_matches(&__handler, h->__exception_type, &adjusted))
    return nullptr;
  return adjusted;
}

}} // namespace __ycxx::__abi
