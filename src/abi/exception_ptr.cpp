// libycxx ABI runtime: the exception_ptr operations (include/ycxx/core/exception_ptr.hpp).
#include "eh.hpp"
#include "internal.hpp"

#include <exception>

namespace ycxx::abi {

void exception_ptr_retain(void* object) noexcept { retain_primary(header_of_object(object)); }

void exception_ptr_release(void* object) noexcept { release(header_of_object(object)); }

void* current_exception_object() noexcept {
  exception_header* h = globals()->caught_exceptions;
  if (!h)
    return nullptr;
  if (!is_native(h->unwind_header.exception_class)) {
    // A foreign exception cannot be referred to: [propagation]/9 lets the result refer to a
    // bad_exception instead.
    void* obj = __cxa_allocate_exception(sizeof(std::bad_exception));
    ::new (obj) std::bad_exception();
    exception_header* b = header_of_object(obj);
    b->exception_type = const_cast<std::type_info*>(&typeid(std::bad_exception));
    b->exception_destructor = [](void* p) { static_cast<std::bad_exception*>(p)->~bad_exception(); };
    b->unwind_header.exception_class = primary_class;
    return obj;
  }
  exception_header* p = retain_primary(h);
  return p + 1;
}

[[noreturn]] void rethrow_exception_object(void* object) { rethrow_primary(object); }

const void* exception_object_as(void* object, const std::type_info& handler) noexcept {
  exception_header* h = header_of_object(object);
  void* adjusted = object;
  if (!catch_matches(&handler, h->exception_type, &adjusted))
    return nullptr;
  return adjusted;
}

} // namespace ycxx::abi
