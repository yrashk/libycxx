// libycxx ABI runtime: the exception-handling entry points under the Itanium C++ ABI's names
// ([ABI-EH] 2.4-2.5). Each forwards to the runtime's own name (entry.hpp, DECISIONS §20.6): in
// static mode the two are in one image; in shared mode this file is the part each image links
// itself, hidden, so that no image exports a name another C++ runtime defines.
//
// Each forwarder is a guaranteed tail call, so that it leaves no frame: the unwinder would
// otherwise step through __cxa_throw's in both phases of every throw (about 20% of a throw and
// catch). The compilers never jump to a noreturn function, so __ycxx_abi_throw and
// __ycxx_abi_rethrow are not declared noreturn, and this file includes no header that declares
// __cxa_throw so (<exception>'s exception_ptr does; std::terminate is cxa_terminate.cpp's).
// __cxa_throw stays in one archive member with __cxa_allocate_exception, which every throw needs:
// AddressSanitizer's runtime defines a weak __cxa_throw of its own (an interceptor that calls the
// "real" one), so a member that held __cxa_throw alone would never be linked and the
// interceptor would find no __cxa_throw to call.
#include <cstddef>
#include <typeinfo>
#include <ycxx/core/hidden_symbol.hpp>

#include "../entry.hpp"

// GCC declares the entry points that its exception-handling code calls itself, with default
// visibility, and keeps that visibility for their definitions (a visibility attribute here is
// ignored, with a warning). Assembler directives hide them, so that a shared object built with
// libycxx never exports half of its runtime: with the rest hidden, a process holding another
// runtime (libstdc++'s) would bind these names to one runtime and the others to the other. Clang
// gets the same directives.
asm((__ycxx::__detail::__hide_symbol("__cxa_allocate_exception")));
asm((__ycxx::__detail::__hide_symbol("__cxa_free_exception")));
asm((__ycxx::__detail::__hide_symbol("__cxa_throw")));
asm((__ycxx::__detail::__hide_symbol("__cxa_begin_catch")));
asm((__ycxx::__detail::__hide_symbol("__cxa_end_catch")));
asm((__ycxx::__detail::__hide_symbol("__cxa_call_unexpected")));
asm((__ycxx::__detail::__hide_symbol("__cxa_call_terminate")));

extern "C" {

void* __cxa_allocate_exception(std::size_t __thrown_size) noexcept {
  __attribute__((__musttail__)) return __ycxx_abi_allocate_exception(__thrown_size);
}
void __cxa_free_exception(void* __thrown) noexcept {
  __attribute__((__musttail__)) return __ycxx_abi_free_exception(__thrown);
}
// (GCC predeclares __cxa_throw with a void* type_info parameter.)
void __cxa_throw(void* __thrown, void* __tinfo, void (*__dest)(void*)) {
  __attribute__((__musttail__)) return __ycxx_abi_throw(__thrown, __tinfo, __dest);
}
[[__gnu__::__visibility__("hidden")]] void __cxa_rethrow() { __attribute__((__musttail__)) return __ycxx_abi_rethrow(); }
void* __cxa_begin_catch(void* __ue) noexcept { __attribute__((__musttail__)) return __ycxx_abi_begin_catch(__ue); }
void __cxa_end_catch() { __attribute__((__musttail__)) return __ycxx_abi_end_catch(); }
[[noreturn]] void __cxa_call_unexpected(void* __ue) noexcept { __ycxx_abi_call_unexpected(__ue); }
[[noreturn]] void __cxa_call_terminate(void* __ue) noexcept { __ycxx_abi_call_terminate(__ue); }

[[__gnu__::__visibility__("hidden")]] void* __cxa_get_exception_ptr(void* __ue) noexcept {
  __attribute__((__musttail__)) return __ycxx_abi_get_exception_ptr(__ue);
}
[[__gnu__::__visibility__("hidden")]] std::type_info* __cxa_current_exception_type() noexcept {
  __attribute__((__musttail__)) return __ycxx_abi_current_exception_type();
}
[[__gnu__::__visibility__("hidden")]] void* __cxa_get_globals() noexcept {
  __attribute__((__musttail__)) return __ycxx_abi_get_globals();
}
[[__gnu__::__visibility__("hidden")]] void* __cxa_get_globals_fast() noexcept {
  __attribute__((__musttail__)) return __ycxx_abi_get_globals_fast();
}

} // extern "C"
