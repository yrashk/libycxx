// libycxx ABI runtime: the exception-handling entry points under the Itanium C++ ABI's names
// ([ABI-EH] 2.4-2.5), and std::terminate. Each forwards to the runtime's own name (entry.hpp,
// DECISIONS §20.6): in static mode the two are in one image; in shared mode this file is the part
// each image links itself, hidden, so that no image exports a name another C++ runtime defines.
#include <cstddef>
#include <exception>
#include <typeinfo>

#include "../entry.hpp"
#include "../internal.hpp"

// GCC declares the entry points that its exception-handling code calls itself, with default
// visibility, and keeps that visibility for their definitions (a visibility attribute here is
// ignored, with a warning). An assembler directive hides them, so that a shared object built with
// libycxx never exports half of its runtime: with the rest hidden, a process holding another
// runtime (libstdc++'s) would bind these names to one runtime and the others to the other. Clang
// gets the same directives.
namespace {
consteval __ycxx::__abi::__asm_text hide_compiler_declared_entry_points() {
  __ycxx::__abi::__asm_text a;
  for (const char* name : {"__cxa_allocate_exception", "__cxa_free_exception", "__cxa_throw", "__cxa_begin_catch",
                           "__cxa_end_catch", "__cxa_call_unexpected", "__cxa_call_terminate"}) {
    // Mach-O symbols carry the C prefix '_'.
    a.append(__ycxx::__detail::__cfg::__darwin ? ".private_extern _" : ".hidden ");
    a.append(name);
    a.append("\n");
  }
  return a;
}
} // namespace
asm((hide_compiler_declared_entry_points()));

extern "C" {

void* __cxa_allocate_exception(std::size_t __thrown_size) noexcept {
  return __ycxx_abi_allocate_exception(__thrown_size);
}
void __cxa_free_exception(void* __thrown) noexcept { __ycxx_abi_free_exception(__thrown); }
// (GCC predeclares __cxa_throw with a void* type_info parameter.)
[[noreturn]] void __cxa_throw(void* __thrown, void* __tinfo, void (*__dest)(void*)) {
  __ycxx_abi_throw(__thrown, __tinfo, __dest);
}
void* __cxa_begin_catch(void* __ue) noexcept { return __ycxx_abi_begin_catch(__ue); }
void __cxa_end_catch() { __ycxx_abi_end_catch(); }
[[noreturn]] void __cxa_call_unexpected(void* __ue) noexcept { __ycxx_abi_call_unexpected(__ue); }
[[noreturn]] void __cxa_call_terminate(void* __ue) noexcept { __ycxx_abi_call_terminate(__ue); }

[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_rethrow() { __ycxx_abi_rethrow(); }
[[__gnu__::__visibility__("hidden")]] void* __cxa_get_exception_ptr(void* __ue) noexcept {
  return __ycxx_abi_get_exception_ptr(__ue);
}
[[__gnu__::__visibility__("hidden")]] std::type_info* __cxa_current_exception_type() noexcept {
  return __ycxx_abi_current_exception_type();
}
[[__gnu__::__visibility__("hidden")]] void* __cxa_get_globals() noexcept { return __ycxx_abi_get_globals(); }
[[__gnu__::__visibility__("hidden")]] void* __cxa_get_globals_fast() noexcept { return __ycxx_abi_get_globals_fast(); }

} // extern "C"

// Clang's __clang_call_terminate calls std::terminate by its mangled name (DECISIONS §20.5).
namespace [[__gnu__::__visibility__("hidden")]] std { // plain std (DECISIONS §20.5)
[[noreturn]] void terminate() noexcept { __ycxx_abi_terminate(); }
} // namespace std
