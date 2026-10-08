// libycxx hosted runtime: abi::__cxa_demangle (<cxxabi.h>, Itanium C++ ABI 3.4) under the ABI's
// name, forwarding to the runtime's own (cxa_demangle.cpp, ../abi/entry.hpp, DECISIONS §20.6).
#include <cstddef>
#include <cxxabi.h>

#include "../abi/entry.hpp"

namespace [[__gnu__::__visibility__("hidden")]] __cxxabiv1 {
extern "C" {
char* __cxa_demangle(const char* __mangled_name, char* __output_buffer, std::size_t* __length, int* __status) {
  return __ycxx_abi_demangle(__mangled_name, __output_buffer, __length, __status);
}
} // extern "C"
} // namespace __cxxabiv1
