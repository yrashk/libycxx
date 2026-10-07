// libycxx hosted runtime: abi::__cxa_demangle (<cxxabi.h>), the Itanium C++ ABI's demangler
// interface (3.4 "Demangler API"), over the runtime's own demangler (demangle.cpp).
//
// mangled_name is a symbol's name ("_Z..."; a clone suffix such as ".cold" is kept, as
// " [clone .cold]") or a type's mangling alone, the form of type_info::name() ("i",
// "St13runtime_error"). The result is NUL-terminated, in output_buffer when it is large enough
// (*length bytes, from malloc), else in output_buffer grown with realloc, or in a new region
// from malloc when output_buffer is null; length, when not null, receives the buffer's size.
// *status: 0 success, -1 allocation failure, -2 not a valid mangled name, -3 invalid argument
// (mangled_name null, or output_buffer given without length). The caller frees the result.
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <new>
#include <string>

#include "demangle.hpp"

namespace [[__gnu__::__visibility__("hidden")]] __cxxabiv1 {
extern "C" {

char* __cxa_demangle(const char* __mangled_name, char* __output_buffer, std::size_t* __length, int* __status) {
  int __st = 0;
  char* __result = nullptr;
  if (__mangled_name == nullptr || (__output_buffer != nullptr && __length == nullptr)) {
    __st = -3;
  } else {
    std::string __text;
    bool __ok = false;
    try {
      __ok = std::strncmp(__mangled_name, "_Z", 2) == 0 ? __ycxx::__detail::__demangle(__mangled_name, __text)
                                                        : __ycxx::__detail::__demangle_type(__mangled_name, __text);
    } catch (const std::bad_alloc&) {
      __st = -1;
    }
    if (__st == 0 && !__ok)
      __st = -2;
    if (__st == 0) {
      const std::size_t __need = __text.size() + 1;
      if (__output_buffer != nullptr && *__length >= __need) {
        __result = __output_buffer;
      } else {
        // realloc of a null pointer is malloc; on failure the caller's buffer stays theirs.
        __result = static_cast<char*>(std::realloc(__output_buffer, __need));
        if (__result == nullptr)
          __st = -1;
        else if (__length != nullptr)
          *__length = __need;
      }
      if (__result != nullptr)
        std::memcpy(__result, __text.c_str(), __need);
    }
  }
  if (__status != nullptr)
    *__status = __st;
  return __result;
}

} // extern "C"
} // namespace __cxxabiv1
