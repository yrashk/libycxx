// <cxxabi.h>: the Itanium C++ ABI's run-time interface that programs call, as libycxx's ABI
// runtime provides it. Not a standard header: GCC's and Clang's C++ libraries provide it, and
// programs use it to demangle type names (GoogleTest, {fmt}, Boost) and to ask the type of the
// exception being handled.
//
//   abi::__cxa_demangle            Itanium C++ ABI 3.4 ("Demangler API"): a mangled name, a
//                                  symbol ("_Z...") or a type ("i", "St13runtime_error", the
//                                  form of type_info::name()), demangled into a buffer from
//                                  malloc (or the caller's, grown with realloc); *status 0
//                                  (success), -1 (allocation failure), -2 (not a valid mangled
//                                  name) or -3 (an invalid argument)
//   abi::__cxa_current_exception_type   the type of the exception currently handled, or null
//
// Like the rest of the ABI runtime, both are hidden in each image that links libycxx (DECISIONS
// §2).
#pragma once

#include <cstddef>
#include <typeinfo>

namespace [[__gnu__::__visibility__("hidden")]] __cxxabiv1 {
extern "C" {

char* __cxa_demangle(const char* __mangled_name, char* __output_buffer, std::size_t* __length, int* __status);
std::type_info* __cxa_current_exception_type() noexcept;

} // extern "C"
} // namespace __cxxabiv1

namespace abi = __cxxabiv1;
