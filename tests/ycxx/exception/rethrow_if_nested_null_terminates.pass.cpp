// [except.nested]/9: rethrow_if_nested(e): for a polymorphic E with an accessible unambiguous
// nested_exception base, "performs: if (auto p = dynamic_cast<const
// nested_exception*>(addressof(e))) p->rethrow_nested();" and /4 rethrow_nested() "If
// nested_ptr() returns a null pointer, the function calls the function std::terminate."
// REQUIRES: exceptions
#include <exception>
#include <cstdlib>
#include <stdexcept>
#include "check.hpp"

struct Mixed : std::runtime_error, std::nested_exception {
  Mixed() : std::runtime_error("mixed") {}  // constructed outside any handler: null nested_ptr
};

int main() {
  std::set_terminate([] { std::_Exit(0); });
  Mixed m;
  CHECK(m.nested_ptr() == nullptr);
  const std::runtime_error& base = m;  // E = runtime_error, polymorphic; cross-cast succeeds
  try {
    std::rethrow_if_nested(base);
  } catch (...) {
    CHECK(false);
  }
  return 1;  // must not return
}
