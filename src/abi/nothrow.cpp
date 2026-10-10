// libycxx ABI runtime: std::nothrow ([new.syn]). Each image has its own, hidden (DECISIONS §20.5:
// libstdc++ and libc++ export _ZSt7nothrow), so in shared mode it is part of libycxx_nonshared.a.
#include <new>

namespace [[__gnu__::__visibility__("hidden")]] std { // plain std (DECISIONS §20.5)
extern const nothrow_t nothrow;
const nothrow_t nothrow{};
} // namespace std
