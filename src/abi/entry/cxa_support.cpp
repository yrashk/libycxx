// libycxx ABI runtime: the helpers the compilers call, under the Itanium C++ ABI's names (pure
// and deleted virtual calls, the throwing helpers of dynamic_cast, typeid and new[], thread_local
// destructor registration), forwarding to the runtime's own (entry.hpp, DECISIONS §20.6).
#include "../entry.hpp"

extern "C" {
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_pure_virtual() { __ycxx_abi_pure_virtual(); }
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_deleted_virtual() { __ycxx_abi_deleted_virtual(); }
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_bad_cast() { __ycxx_abi_bad_cast(); }
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_bad_typeid() { __ycxx_abi_bad_typeid(); }
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_throw_bad_array_new_length() {
  __ycxx_abi_throw_bad_array_new_length();
}
[[__gnu__::__visibility__("hidden")]] int __cxa_thread_atexit(void (*__dtor)(void*), void* __obj, void* __dso) noexcept {
  return __ycxx_abi_thread_atexit(__dtor, __obj, __dso);
}
} // extern "C"
