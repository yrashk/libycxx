// [except.throw]/9 [Note 7]: "If a destructor directly invoked by stack unwinding exits via an
// exception, std::terminate is invoked." [except.terminate]/1.4. The destructor here is
// noexcept(false), so termination is due to unwinding, not to the exception specification.
#include <cstdlib>
#include <exception>
#include "check.hpp"

[[noreturn]] void on_terminate() {
  // the second exception is the active one, or the first: the program must not continue
  std::_Exit(0);
}

struct Bomb {
  ~Bomb() noexcept(false) { throw 2; }
};

int main() {
  std::set_terminate(on_terminate);
  // outside unwinding, such a destructor may throw normally
  int which = 0;
  try {
    Bomb b;
  } catch (int i) {
    which = i;
  }
  CHECK(which == 2);
  try {
    Bomb b;
    throw 1;
  } catch (...) {
    return 2;
  }
  return 1;
}
