// libycxx ABI runtime: std::terminate ([except.terminate]), forwarding to the runtime's own
// (entry.hpp, DECISIONS §20.6). In plain std: Clang's __clang_call_terminate calls it by its
// mangled name, _ZSt9terminatev (DECISIONS §20.5).
#include <exception>

#include "../entry.hpp"

namespace [[__gnu__::__visibility__("hidden")]] std { // plain std (DECISIONS §20.5)
[[noreturn]] void terminate() noexcept { __ycxx_abi_terminate(); }
} // namespace std
