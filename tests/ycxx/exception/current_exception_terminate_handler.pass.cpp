// [except.handle]/9: "an implicit handler is considered active when the function
// std::terminate is entered due to a throw." /10: the exception with the most recently
// activated handler that is still active is the currently handled exception. /7: when the
// search for a handler exits a function with a non-throwing exception specification,
// std::terminate is invoked. So inside the terminate handler, current_exception() refers to
// the exception that caused the call ([propagation]/9).
#include <exception>
#include <cstdlib>
#include "check.hpp"

[[noreturn]] void on_terminate() {
  std::exception_ptr p = std::current_exception();
  if (!p) std::_Exit(2);
  try {
    std::rethrow_exception(p);
  } catch (int i) {
    std::_Exit(i == 42 ? 0 : 3);
  } catch (...) {
    std::_Exit(4);
  }
  std::_Exit(5);
}

void (*volatile thrower)() = [] { throw 42; };

void call_noexcept() noexcept { thrower(); }

int main() {
  std::set_terminate(on_terminate);
  call_noexcept();
  return 1;  // not reached
}
