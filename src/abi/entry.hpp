// libycxx ABI runtime: the runtime's own names for its entry points (DECISIONS §20.6).
//
// The compilers call the Itanium C++ ABI's entry points by their names (__cxa_throw,
// __gxx_personality_v0, __dynamic_cast, ...), and so do other C++ runtimes' libraries, which
// define the same names. libycxx's runtime implements each under a name only libycxx uses,
// __ycxx_abi_<name>, declared here with the library's visibility (_YCXX_VISIBILITY: hidden in
// static mode, exported from the shared library in shared mode). The ABI's names are thin hidden
// forwarders in src/abi/entry/, one file per implementation file, so that a static link pulls the
// same archive members as before; in shared mode they are the part of the runtime each image
// links itself, while the implementations, and the runtime's state, are the shared library's.
#pragma once

#include <unwind.h>

#include <cstddef>
#include <cstdint>
#include <typeinfo>


extern "C" {

// Exception handling (exception.cpp; [ABI-EH] 2.4-2.5).
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void* __ycxx_abi_allocate_exception(std::size_t __thrown_size) noexcept;
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_free_exception(void* __thrown) noexcept;
// __ycxx_abi_throw and __ycxx_abi_rethrow never return, but are not declared [[noreturn]]: the
// compilers never turn a call to a noreturn function into a jump, and the forwarders of the throw
// path must leave no frame of their own for the unwinder to step through (two more frame steps
// per throw, about 20% of a throw and catch; src/abi/entry/cxa_exception.cpp).
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_throw(void* __thrown, void* __tinfo, void (*__dest)(void*));
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_rethrow();
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void* __ycxx_abi_begin_catch(void* __ue) noexcept;
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_end_catch();
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void* __ycxx_abi_get_exception_ptr(void* __ue) noexcept;
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std::type_info* __ycxx_abi_current_exception_type() noexcept;
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void* __ycxx_abi_get_globals() noexcept;
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void* __ycxx_abi_get_globals_fast() noexcept;
[[noreturn, __gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_call_unexpected(void* __ue) noexcept;
[[noreturn, __gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_call_terminate(void* __ue) noexcept;
// std::terminate ([except.terminate]), which Clang calls by its mangled name.
[[noreturn, __gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_terminate() noexcept;

// The personality routine (personality.cpp; [ABI-EH] 2.5.2).
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] _Unwind_Reason_Code
__ycxx_abi_personality(int __version, _Unwind_Action __actions, std::uint64_t __cls, _Unwind_Exception* __ue,
                       _Unwind_Context* __ctx);

// dynamic_cast (dynamic_cast.cpp; 2.9.7). src and dst are __class_type_info objects.
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void* __ycxx_abi_dynamic_cast(const void* __sub, const void* __src,
                                                                          const void* __dst,
                                                                          std::ptrdiff_t __src2dst_offset);

// Static-local guards (guard.cpp; 3.3.3).
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] int __ycxx_abi_guard_acquire(std::int64_t* __g);
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_guard_release(std::int64_t* __g) noexcept;
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_guard_abort(std::int64_t* __g) noexcept;

// The helpers the compilers call (support.cpp; 3.2.6, 3.3.5, 2.9.6).
[[noreturn, __gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_pure_virtual();
[[noreturn, __gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_deleted_virtual();
[[noreturn, __gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_bad_cast();
[[noreturn, __gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_bad_typeid();
[[noreturn, __gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_throw_bad_array_new_length();
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] int __ycxx_abi_thread_atexit(void (*__dtor)(void*), void* __obj,
                                                                         void* __dso) noexcept;

// The registration of an image's copy of the RTTI classes (rtti_classes.cpp, rtti.cpp; DECISIONS
// §20.6): `__types` lists the type_info objects of the image's nine concrete ABI classes, in the
// order of __ycxx::__abi::__rtti_class_kinds (rtti.hpp); unregistered, with the same list, when
// the image is unloaded. A full registry leaves the image's objects to be classified by name.
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_rtti_register(const std::type_info* const* __types) noexcept;
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] void __ycxx_abi_rtti_unregister(const std::type_info* const* __types) noexcept;

// The demangler (src/hosted/cxa_demangle.cpp; 3.4).
[[__gnu__::__visibility__(_YCXX_VISIBILITY)]] char* __ycxx_abi_demangle(const char* __mangled_name,
                                                                      char* __output_buffer,
                                                                      std::size_t* __length, int* __status);

} // extern "C"
