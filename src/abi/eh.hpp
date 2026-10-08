// libycxx ABI runtime: the exception object header and the per-thread exception state
// (Itanium C++ ABI, exception handling, 2.2). Shared by the throw/catch entry points
// (exception.cpp), the personality routine (personality.cpp) and exception_ptr.
#pragma once

#include <unwind.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <typeinfo>

#include "entry.hpp" // the runtime's entry points, used across its translation units

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __abi {

// "XXXXC++\0" ([ABI-EH] 2.4.3), with the vendor string "YCXX". The last byte distinguishes a
// dependent exception (a rethrown exception_ptr, which refers to a primary exception's object).
inline constexpr std::uint64_t __primary_class = 0x5943'5858'432B'2B00; // "YCXXC++\0"
inline constexpr std::uint64_t __dependent_class = __primary_class | 1;  // "YCXXC++\1"

// Precedes the thrown object. The part from exception_type to the end follows [ABI-EH] 2.2.1;
// unwind_header is last, so the thrown object immediately follows it (compilers rely on this:
// Clang addresses the object of `catch (_Tp*&)` as the unwind header plus its size).
//
// A primary exception owns its object. reference_count counts its owners: the throw/catch
// machinery (one reference from the throw until the last handler finishes) and each
// exception_ptr. A dependent exception (thrown by rethrow_exception) has no object of its own:
// it holds one reference to its primary, and its type/destructor fields are copies.
struct __eh_globals;
struct __exception_header {
  std::size_t __reference_count;
  std::size_t __allocation_size; // 0: from the emergency pool
  void* __primary_object;        // dependent exceptions: the primary's thrown object
  // The uncaught-exception count this exception was added to by its throw or rethrow, and is
  // removed from when it is caught. Every image linking libycxx has its own runtime (DECISIONS
  // §2), and the handler may be in another such image than the throw, whose own count must not
  // be decremented for it.
  __eh_globals* __counted_in;

  std::type_info* __exception_type;
  void (*__exception_destructor)(void*);
  void (*unexpected_handler)();
  std::terminate_handler terminate_handler;
  __exception_header* __next_exception;
  int __handler_count;
  int __handler_switch_value;
  const unsigned char* __action_record;
  const unsigned char* __lsda;
  void* __catch_temp; // the landing pad found in phase 1
  void* __adjusted_ptr;
  _Unwind_Exception __unwind_header;
};
static_assert(sizeof(__exception_header) % alignof(_Unwind_Exception) == 0,
              "the thrown object must directly follow the unwind header");

inline __exception_header* __header_of_object(void* __thrown) noexcept {
  return static_cast<__exception_header*>(__thrown) - 1;
}
inline __exception_header* __header_of_unwind(_Unwind_Exception* __ue) noexcept {
  return reinterpret_cast<__exception_header*>(reinterpret_cast<unsigned char*>(__ue + 1) - sizeof(__exception_header));
}
inline bool __is_native(std::uint64_t __cls) noexcept { return (__cls & ~std::uint64_t(1)) == __primary_class; }
inline bool __is_dependent(const __exception_header* h) noexcept {
  return h->__unwind_header.exception_class == __dependent_class;
}
// The thrown object of a native exception.
inline void* object_of(__exception_header* h) noexcept { return __is_dependent(h) ? h->__primary_object : h + 1; }

// The language-specific data area of a frame. _Unwind_GetLanguageSpecificData returns void* in
// GCC's and LLVM's <unwind.h> and uintptr_t in Apple's: overloads accept either.
inline const unsigned char* __lsda_bytes(void* p) noexcept { return static_cast<const unsigned char*>(p); }
inline const unsigned char* __lsda_bytes(std::uintptr_t __v) noexcept { return reinterpret_cast<const unsigned char*>(__v); }
inline const unsigned char* __lsda_of(_Unwind_Context* __ctx) noexcept {
  return __ycxx::__abi::__lsda_bytes(_Unwind_GetLanguageSpecificData(__ctx));
}

// [ABI-EH] 2.2.2.
struct __eh_globals {
  __exception_header* __caught_exceptions;
  unsigned int uncaught_exceptions;
};
__eh_globals* __globals() noexcept;

// Drops one reference to a primary exception (destroying and freeing it at zero), or finishes a
// dependent one (freeing it and dropping its reference to the primary).
void release(__exception_header* h) noexcept;
// Adds a reference to the primary exception of h.
__exception_header* __retain_primary(__exception_header* h) noexcept;

// rethrow_exception: throws a dependent exception referring to a primary exception's object.
[[noreturn]] void __rethrow_primary(void* __primary_object);

// std::terminate for an exception the runtime cannot handle: marks it caught first, so the
// terminate handler sees it as the current exception ([except.handle]/9).
[[noreturn]] void __terminate_for(_Unwind_Exception* __ue) noexcept;

}} // namespace __ycxx::__abi
