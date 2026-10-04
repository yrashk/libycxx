// [except.throw]/9: "If the exception handling mechanism handling an uncaught exception
// directly invokes a function that exits via an exception, the function std::terminate is
// invoked." The copy-initialization of a handler's parameter happens before the handler is
// active (/6, [except.handle]/9), so a copy constructor that throws there terminates. A copy
// from a base subobject cannot be elided ([class.copy.elision]/1.4 needs the same type).
#include <cstdlib>
#include <exception>
#include "check.hpp"

[[noreturn]] void on_terminate() { std::_Exit(0); }

struct C {
  C() = default;
  C(const C&) {
    if (std::uncaught_exceptions()) throw 0;
  }
};
struct D : C {};

int main() {
  std::set_terminate(on_terminate);
  try {
    throw D();
  } catch (C) {
    return 2;
  } catch (...) {
    return 3;
  }
  return 1;
}
