// [except.ctor]/4: "If the compound-statement of the function-body of a delegating
// constructor for an object exits via an exception, the object's destructor is invoked. Such
// destruction is sequenced before entering a handler of the function-try-block of a
// delegating constructor for that object, if any." If the target constructor throws, /3
// applies instead (subobjects only, no destructor).
// REQUIRES: exceptions
#include "check.hpp"

static int dtor_calls = 0, member_dtors = 0;
static int dtor_before_handler = -1;

struct M {
  ~M() { ++member_dtors; }
};

struct X {
  M m;
  X(int v) {
    if (v == 1) throw 1;
  }
  X(int v, int) : X(v) { throw 2; }
  X(double) try : X(0) {
    throw 3;
  } catch (int) {
    dtor_before_handler = dtor_calls;
  }
  ~X() { ++dtor_calls; }
};

int main() {
  int which = 0;
  try {
    X x(0, 0);
  } catch (int i) {
    which = i;
  }
  CHECK(which == 2);
  CHECK(dtor_calls == 1 && member_dtors == 1);

  dtor_calls = member_dtors = 0;
  which = 0;
  try {
    X x(1, 0);  // target constructor throws: X's destructor is not invoked
  } catch (int i) {
    which = i;
  }
  CHECK(which == 1);
  CHECK(dtor_calls == 0 && member_dtors == 1);

  dtor_calls = member_dtors = 0;
  which = 0;
  try {
    X x(1.0);
  } catch (int i) {
    which = i;  // rethrown at the end of the constructor's handler
  }
  CHECK(which == 3);
  CHECK(dtor_before_handler == 1);
  CHECK(dtor_calls == 1);
  return 0;
}
