// [except.handle]/7: "If the search for a handler exits the function body of a function with
// a non-throwing exception specification, the function std::terminate is invoked." This holds
// even though an outer handler would match. [except.terminate]/1.3.
// REQUIRES: exceptions
#include <cstdlib>
#include <exception>
#include "check.hpp"

[[noreturn]] void on_terminate() { std::_Exit(0); }

static void may_throw(int k) {
  if (k > 0) throw k;
}

static void wrapper(int k) noexcept { may_throw(k); }

int main() {
  std::set_terminate(on_terminate);
  wrapper(0);  // no exception: no termination
  try {
    wrapper(1);
  } catch (...) {
    return 2;  // must not be reached
  }
  return 1;
}
