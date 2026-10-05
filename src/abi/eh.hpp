// libycxx ABI runtime: the exception object header and the per-thread exception state
// (Itanium C++ ABI, exception handling, 2.2). Shared by the throw/catch entry points
// (exception.cpp), the personality routine (personality.cpp) and exception_ptr.
#pragma once

#include <unwind.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <typeinfo>

namespace ycxx::abi {

// "XXXXC++\0" ([ABI-EH] 2.4.3), with the vendor string "YCXX". The last byte distinguishes a
// dependent exception (a rethrown exception_ptr, which refers to a primary exception's object).
inline constexpr std::uint64_t primary_class = 0x5943'5858'432B'2B00; // "YCXXC++\0"
inline constexpr std::uint64_t dependent_class = primary_class | 1;  // "YCXXC++\1"

// Precedes the thrown object. The part from exception_type to the end follows [ABI-EH] 2.2.1;
// unwind_header is last, so the thrown object immediately follows it (compilers rely on this:
// Clang addresses the object of `catch (T*&)` as the unwind header plus its size).
//
// A primary exception owns its object. reference_count counts its owners: the throw/catch
// machinery (one reference from the throw until the last handler finishes) and each
// exception_ptr. A dependent exception (thrown by rethrow_exception) has no object of its own:
// it holds one reference to its primary, and its type/destructor fields are copies.
struct exception_header {
  std::size_t reference_count;
  std::size_t allocation_size; // 0: from the emergency pool
  void* primary_object;        // dependent exceptions: the primary's thrown object

  std::type_info* exception_type;
  void (*exception_destructor)(void*);
  void (*unexpected_handler)();
  std::terminate_handler terminate_handler;
  exception_header* next_exception;
  int handler_count;
  int handler_switch_value;
  const unsigned char* action_record;
  const unsigned char* lsda;
  void* catch_temp; // the landing pad found in phase 1
  void* adjusted_ptr;
  _Unwind_Exception unwind_header;
};
static_assert(sizeof(exception_header) % alignof(_Unwind_Exception) == 0,
              "the thrown object must directly follow the unwind header");

inline exception_header* header_of_object(void* thrown) noexcept {
  return static_cast<exception_header*>(thrown) - 1;
}
inline exception_header* header_of_unwind(_Unwind_Exception* ue) noexcept {
  return reinterpret_cast<exception_header*>(reinterpret_cast<unsigned char*>(ue + 1) - sizeof(exception_header));
}
inline bool is_native(std::uint64_t cls) noexcept { return (cls & ~std::uint64_t(1)) == primary_class; }
inline bool is_dependent(const exception_header* h) noexcept {
  return h->unwind_header.exception_class == dependent_class;
}
// The thrown object of a native exception.
inline void* object_of(exception_header* h) noexcept { return is_dependent(h) ? h->primary_object : h + 1; }

// The language-specific data area of a frame. _Unwind_GetLanguageSpecificData returns void* in
// GCC's and LLVM's <unwind.h> and uintptr_t in Apple's: overloads accept either.
inline const unsigned char* lsda_bytes(void* p) noexcept { return static_cast<const unsigned char*>(p); }
inline const unsigned char* lsda_bytes(std::uintptr_t v) noexcept { return reinterpret_cast<const unsigned char*>(v); }
inline const unsigned char* lsda_of(_Unwind_Context* ctx) noexcept {
  return ycxx::abi::lsda_bytes(_Unwind_GetLanguageSpecificData(ctx));
}

// [ABI-EH] 2.2.2.
struct eh_globals {
  exception_header* caught_exceptions;
  unsigned int uncaught_exceptions;
};
eh_globals* globals() noexcept;

// Drops one reference to a primary exception (destroying and freeing it at zero), or finishes a
// dependent one (freeing it and dropping its reference to the primary).
void release(exception_header* h) noexcept;
// Adds a reference to the primary exception of h.
exception_header* retain_primary(exception_header* h) noexcept;

// rethrow_exception: throws a dependent exception referring to a primary exception's object.
[[noreturn]] void rethrow_primary(void* primary_object);

// std::terminate for an exception the runtime cannot handle: marks it caught first, so the
// terminate handler sees it as the current exception ([except.handle]/9).
[[noreturn]] void terminate_for(_Unwind_Exception* ue) noexcept;

} // namespace ycxx::abi

// [ABI-EH] entry points defined by the runtime and used across its translation units.
extern "C" {
void* __cxa_begin_catch(void* unwind_exception) noexcept;
void __cxa_end_catch();
void* __cxa_allocate_exception(std::size_t thrown_size) noexcept;
void __cxa_free_exception(void* thrown_exception) noexcept;
}
