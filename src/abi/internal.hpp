// libycxx ABI runtime: interfaces shared between its translation units. Not installed.
//
// The runtime implements the Itanium C++ ABI's language-support layer (exception handling,
// RTTI, dynamic_cast, static-local guards) from the published ABI documents. Unwinding itself
// comes from the toolchain's unwinder (libgcc_s / libgcc_eh) through <unwind.h>.
#pragma once

#include <cstddef>
#include <exception>
#include <typeinfo>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __abi {

// Exception handler matching ([except.handle]/3), used by the personality routine and by
// exception_ptr_cast.
//   handler: the type_info of the handler's type, as the compiler records it in the exception
//            table (for `catch (_Tp&)` and `catch (_Tp)` this is T without top-level cv).
//   thrown:  the type_info of the exception object's static type at the throw.
//   *obj:    on entry, the address of the exception object. On a match it is set to the value
//            __cxa_begin_catch must return: the address of the handler's base-class subobject
//            for class types, the converted pointer value itself for pointer handlers, and the
//            object's address otherwise.
// Returns whether the handler matches.
bool __catch_matches(const std::type_info* __handler, const std::type_info* __thrown, void** __obj) noexcept;

// Assembler text built during constant evaluation, for `asm((...))`: the directives that hide the
// symbols GCC gives default visibility despite a visibility attribute (DECISIONS §2).
struct __asm_text {
  char __text[16384]{};
  std::size_t length = 0;
  constexpr void append(const char* s) noexcept {
    while (*s)
      __text[length++] = *s++;
  }
  constexpr const char* data() const noexcept { return __text; }
  constexpr std::size_t size() const noexcept { return length; }
};

}} // namespace __ycxx::__abi
