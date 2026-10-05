// [except.nested]/4: rethrow_nested() "Effects: If nested_ptr() returns a null pointer, the
// function calls the function std::terminate. Otherwise, it throws the stored exception
// captured by *this." A nested_exception constructed while no exception is handled stores a
// null exception_ptr (/3).
// REQUIRES: exceptions
#include <exception>
#include <cstdlib>
#include "check.hpp"

int main() {
  std::set_terminate([] { std::_Exit(0); });
  std::nested_exception ne;
  CHECK(ne.nested_ptr() == nullptr);
  try {
    ne.rethrow_nested();
  } catch (...) {
    CHECK(false);  // must not throw anything
  }
  return 1;  // must not return
}
