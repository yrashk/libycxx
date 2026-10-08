// libycxx ABI runtime: __cxa_throw and __cxa_rethrow under the Itanium C++ ABI's names ([ABI-EH]
// 2.4.3, 2.5.4), forwarding to the runtime's own (entry.hpp, DECISIONS §20.6). A tail call each,
// so that the forwarder leaves no frame for the unwinder to step through in both phases: neither
// side is declared noreturn (the compilers never jump to a noreturn function), and this file
// sees no header that declares them so (<exception>'s exception_ptr does).
#include <cstddef>
#include <ycxx/core/hidden_symbol.hpp>

#include "../entry.hpp"

// GCC declares __cxa_throw itself, with default visibility, and keeps it for the definition
// (cxa_exception.cpp hides its other such entry points the same way).
asm((__ycxx::__detail::__hide_symbol("__cxa_throw")));

extern "C" {
// (GCC predeclares __cxa_throw with a void* type_info parameter.)
void __cxa_throw(void* __thrown, void* __tinfo, void (*__dest)(void*)) {
  __attribute__((__musttail__)) return __ycxx_abi_throw(__thrown, __tinfo, __dest);
}
[[__gnu__::__visibility__("hidden")]] void __cxa_rethrow() {
  __attribute__((__musttail__)) return __ycxx_abi_rethrow();
}
} // extern "C"
