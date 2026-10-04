// [expr.throw]/3: "If no exception is presently being handled, the function std::terminate
// is invoked." [except.terminate]/1.8. After a handler has exited, its exception is no longer
// being handled.
#include <cstdlib>
#include <exception>
#include "check.hpp"

[[noreturn]] void on_terminate() { std::_Exit(0); }

static void rethrow() { throw; }

int main() {
  std::set_terminate(on_terminate);
  try {
    throw 1;
  } catch (int) {
  }
  try {
    rethrow();
  } catch (...) {
    return 2;
  }
  return 1;
}
