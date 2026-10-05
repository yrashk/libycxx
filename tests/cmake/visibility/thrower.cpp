// A shared library built with libycxx whose exceptions are caught by a program also built with
// libycxx (catcher.cpp). Each links its own hidden copy of the runtime (DECISIONS §2).
#include <exception>
#include <stdexcept>
#include "thrower.hpp"

extern "C" void throw_runtime_error() { throw std::runtime_error("from the library"); }
extern "C" void throw_int() { throw 7; }
extern "C" void throw_derived() { throw derived{}; }
extern "C" void throw_derived_pointer() {
  static derived d;
  throw &d;
}
extern "C" int library_uncaught_exceptions() { return std::uncaught_exceptions(); }
