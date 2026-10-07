// The shared library's interface: standard library types cross it, and so do exceptions.
// GREET_API marks what the library exports. With GCC that is required for a function whose
// signature names a standard library type: libycxx's types have hidden visibility, and GCC gives
// such a function hidden visibility unless it is declared with default visibility itself
// (docs/BUILDING_PROJECTS.md, "Shared libraries").
#pragma once
#include <string>
#include <vector>

#define GREET_API [[gnu::visibility("default")]]

GREET_API std::string greet(const std::vector<std::string>& names);
GREET_API void greet_throw(int code);          // throws std::runtime_error
extern "C" int greet_c_part(int);              // implemented in C
