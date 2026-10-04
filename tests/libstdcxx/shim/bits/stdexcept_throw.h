// Test-harness shim (see c++config.h): the throw helper testsuite_hooks.h calls.
#pragma once
#include <stdexcept>
namespace std {
#if __cpp_exceptions
[[noreturn]] inline void __throw_runtime_error(const char* s) { throw runtime_error(s); }
#else
// Under -fno-exceptions (e.g. 18_support/exception_ptr/64241.cc) a throw does not compile;
// libstdc++'s own helper aborts in that mode, and so does this one.
[[noreturn]] inline void __throw_runtime_error(const char*) { __builtin_abort(); }
#endif
} // namespace std
