// libycxx ABI runtime: the static-local guards under the Itanium C++ ABI's names (3.3.3),
// forwarding to the runtime's own (entry.hpp, DECISIONS §20.6).
#include <cstdint>

#include "../entry.hpp"

extern "C" {
[[__gnu__::__visibility__("hidden")]] int __cxa_guard_acquire(std::int64_t* __g) { return __ycxx_abi_guard_acquire(__g); }
[[__gnu__::__visibility__("hidden")]] void __cxa_guard_release(std::int64_t* __g) noexcept {
  __ycxx_abi_guard_release(__g);
}
[[__gnu__::__visibility__("hidden")]] void __cxa_guard_abort(std::int64_t* __g) noexcept { __ycxx_abi_guard_abort(__g); }
} // extern "C"
