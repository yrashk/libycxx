// [expr.new]/8 and [new.badlength]: a new-expression whose array bound is erroneous (e.g.
// negative, or the size overflows) throws an exception of a type that would match a handler
// of type std::bad_array_new_length. bad_array_new_length derives from bad_alloc.
// XFAIL-COMPILER: clang  clang lowers an erroneous runtime array bound to operator new(SIZE_MAX) (bad_alloc) instead of throwing bad_array_new_length
#include <new>
#include "check.hpp"

int main() {
  volatile int n = -1;
  bool caught = false;
  try {
    int* p = new int[n];
    delete[] p;
  } catch (const std::bad_array_new_length& e) {
    caught = e.what() != nullptr;
  }
  CHECK(caught);
  caught = false;
  try {
    int* p = new int[n];
    delete[] p;
  } catch (const std::bad_alloc&) {
    caught = true;
  }
  CHECK(caught);
  return 0;
}
