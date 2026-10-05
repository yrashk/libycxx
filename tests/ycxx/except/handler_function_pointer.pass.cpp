// [expr.throw]/2: the function-to-pointer conversion is performed on the operand.
// [except.handle]/2: "A handler of type ... function type T is adjusted to be of type
// 'pointer to T'." /3.3.2: a pointer handler matches via "a function pointer conversion"
// ([conv.fctptr]: pointer to noexcept function to pointer to function). [Note 1]: "A handler
// of reference to array or function type is never a match for any exception object."
// REQUIRES: exceptions
// XFAIL: gcc  GCC 16 records catch (int(&)()) as a handler for int(*)(); the runtime cannot tell ([except.handle] Note 1; STATUS)
#include "check.hpp"

int f1() { return 1; }
int f2() noexcept { return 2; }
long f3() { return 3; }

template <class H, class E>
int catches(E e) {
  try {
    try {
      throw e;
    } catch (H) {
      return 1;
    }
  } catch (...) {
    return 0;
  }
  return -1;
}

int main() {
  int which = 0;
  try {
    throw f1;  // decays to int(*)()
  } catch (int (*p)()) {
    which = p();
  }
  CHECK(which == 1);

  which = 0;
  try {
    throw f2;
  } catch (int (*p)()) {  // noexcept dropped
    which = p();
  }
  CHECK(which == 2);

  which = 0;
  try {
    throw f1;
  } catch (int p()) {  // adjusted to int(*)()
    which = p();
  }
  CHECK(which == 1);

  which = 0;
  try {
    throw &f2;
  } catch (int (*const& p)() noexcept) {
    which = p();
  }
  CHECK(which == 2);

  CHECK(catches<int (*)() noexcept>(&f1) == 0);  // cannot add noexcept
  CHECK(catches<long (*)()>(&f1) == 0);
  CHECK(catches<int (*)(int)>(&f1) == 0);
  CHECK(catches<int (*)()>(&f3) == 0);

  // reference to function handler never matches
  which = 0;
  try {
    try {
      throw f1;
    } catch (int (&)()) {
      which = -1;
    }
  } catch (int (*)()) {
    which = 1;
  }
  CHECK(which == 1);
  return 0;
}
