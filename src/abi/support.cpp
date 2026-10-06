// libycxx ABI runtime: pure/deleted virtual calls, the helpers compilers call to throw
// (bad_cast, bad_typeid, bad_array_new_length), thread_local destructor registration, the new
// handler, and std::nothrow. (The static-local guards are guard.cpp's.)
#include <exception>
#include <new>
#include <typeinfo>
#include <ycxx/pal.h>

namespace {

std::new_handler new_handler_v;

[[noreturn]] void fatal(const char* __msg) noexcept { ycxx_pal_abort(__msg); }

} // namespace

extern "C" {

[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_pure_virtual() { fatal("pure virtual function called"); }
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_deleted_virtual() { fatal("deleted virtual function called"); }

[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_bad_cast() { throw std::bad_cast(); }
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_bad_typeid() { throw std::bad_typeid(); }
[[noreturn, __gnu__::__visibility__("hidden")]] void __cxa_throw_bad_array_new_length() { throw std::bad_array_new_length(); }

[[__gnu__::__visibility__("hidden")]] int __cxa_thread_atexit(void (*dtor)(void*), void* __obj, void* __dso) noexcept {
  if (ycxx_pal_thread_atexit(dtor, __obj, __dso) != 0)
    fatal("cannot register a thread_local destructor");
  return 0;
}

} // extern "C"

namespace [[__gnu__::__visibility__("hidden")]] std {

const nothrow_t nothrow{};

new_handler set_new_handler(new_handler __f) noexcept { return __atomic_exchange_n(&new_handler_v, __f, __ATOMIC_ACQ_REL); }
new_handler get_new_handler() noexcept { return __atomic_load_n(&new_handler_v, __ATOMIC_ACQUIRE); }

} // namespace std
