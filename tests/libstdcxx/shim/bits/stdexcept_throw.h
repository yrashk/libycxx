// Test-harness shim (see c++config.h): the throw helper testsuite_hooks.h calls.
#pragma once
#include <stdexcept>
namespace std {
[[noreturn]] inline void __throw_runtime_error(const char* s) { throw runtime_error(s); }
} // namespace std
