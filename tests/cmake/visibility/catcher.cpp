// A program built with libycxx catching the exceptions of a shared library built with libycxx
// (thrower.cpp): the handlers match the library's type_info objects (instances of the library's
// own copy of the runtime's classes), and each runtime's uncaught_exceptions() is 0 afterwards.
// Prints "caught 15 uncaught 0 0".
#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>
#include "thrower.hpp"

int main() {
  int r = 0;
  try {
    throw_runtime_error();
  } catch (const std::exception& e) {
    r |= std::strcmp(e.what(), "from the library") == 0 && std::current_exception() ? 1 : 0;
  } catch (...) {
  }
  try {
    throw_int();
  } catch (int v) {
    r |= v == 7 ? 2 : 0;
  } catch (...) {
  }
  try {
    throw_derived();
  } catch (const base& b) {
    r |= b.b == 1 ? 4 : 0;
  } catch (...) {
  }
  try {
    throw_derived_pointer();
  } catch (base* b) {
    r |= b->b == 1 ? 8 : 0;
  } catch (...) {
  }
  std::printf("caught %d uncaught %d %d\n", r, std::uncaught_exceptions(), library_uncaught_exceptions());
  return r == 15 && std::uncaught_exceptions() == 0 && library_uncaught_exceptions() == 0 ? 0 : 1;
}
