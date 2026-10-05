// [except.terminate]/2: "In all other situations, the stack shall not be unwound before the
// function std::terminate is invoked." For `throw;` with no current exception
// ([except.terminate]/1.8), destructors of live automatic objects must not run before the
// terminate handler is called.
// REQUIRES: exceptions
#include <cstdlib>
#include <exception>
#include "check.hpp"

static volatile int unwound = 0;

[[noreturn]] void on_terminate() { std::_Exit(unwound ? 3 : 0); }

struct Probe {
  ~Probe() { unwound = 1; }
};

static void rethrow() {
  Probe p;
  throw;
}

int main() {
  std::set_terminate(on_terminate);
  try {
    Probe outer;
    rethrow();
  } catch (...) {
    return 2;
  }
  return 1;
}
