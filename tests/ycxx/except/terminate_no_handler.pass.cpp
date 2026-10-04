// [except.handle]/8: "If no matching handler is found, the function std::terminate is
// invoked". [except.terminate]/1.2. The installed terminate handler ([set.terminate]) is
// called; this one exits successfully, so reaching the end of main is a failure.
#include <cstdlib>
#include <exception>
#include "check.hpp"

struct Unmatched {};

[[noreturn]] void on_terminate() { std::_Exit(0); }

static void thrower() {
  try {
    throw Unmatched();
  } catch (int) {  // no match: the search continues and fails
  }
}

int main() {
  std::set_terminate(on_terminate);
  thrower();
  return 1;
}
