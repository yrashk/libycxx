// [class.dtor]/5 + [except.spec]: a destructor without an explicit noexcept-specifier has a
// non-throwing exception specification (here: no potentially-throwing subobject destructors).
// A throw escaping it exits the function body of a non-throwing function, so
// [except.handle]/7: std::terminate is invoked, even with a matching handler outside.
// REQUIRES: exceptions
#include <cstdlib>
#include <exception>
#include "check.hpp"

[[noreturn]] void on_terminate() { std::_Exit(0); }

static volatile int k = 1;
struct Throws {
  ~Throws() {
    if (k) throw 1;  // compilers may warn
  }
};

int main() {
  std::set_terminate(on_terminate);
  try {
    Throws t;
  } catch (...) {
    return 2;
  }
  return 1;
}
