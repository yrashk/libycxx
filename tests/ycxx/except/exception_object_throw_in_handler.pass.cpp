// [except.throw]/4: the exception object is destroyed "when an active handler for the
// exception exits by any means other than rethrowing". A handler exited by throwing a new
// exception is such a means: the old exception object is destroyed before the next handler
// (for the new exception) is entered. [except.handle]/10: inside a nested handler the
// currently handled exception is the most recently activated one still active; after the
// nested handler exits, `throw;` rethrows the outer one again.
// REQUIRES: exceptions
#include "check.hpp"

static int live_a = 0, live_b = 0;
struct A {
  A() { ++live_a; }
  A(const A&) { ++live_a; }
  ~A() { --live_a; }
};
struct B {
  B() { ++live_b; }
  B(const B&) { ++live_b; }
  ~B() { --live_b; }
};

int main() {
  int which = 0;
  try {
    try {
      throw A();
    } catch (A&) {
      CHECK(live_a == 1);
      throw B();
    }
  } catch (B&) {
    CHECK(live_a == 0);  // old exception gone
    CHECK(live_b == 1);
    which = 1;
  }
  CHECK(which == 1 && live_a == 0 && live_b == 0);

  // nested handling inside a handler, then rethrow of the outer exception
  which = 0;
  try {
    try {
      throw A();
    } catch (A&) {
      try {
        throw B();
      } catch (B&) {
        CHECK(live_a == 1 && live_b == 1);
      }
      CHECK(live_b == 0 && live_a == 1);
      throw;  // rethrows A, not B
    }
  } catch (B&) {
    which = -1;
  } catch (A&) {
    which = 2;
    CHECK(live_a == 1);
  }
  CHECK(which == 2 && live_a == 0);

  // rethrow inside a nested handler rethrows the inner exception
  which = 0;
  try {
    try {
      throw A();
    } catch (A&) {
      try {
        throw B();
      } catch (B&) {
        throw;
      }
    }
  } catch (A&) {
    which = -1;
  } catch (B&) {
    which = 3;
    CHECK(live_a == 0 && live_b == 1);
  }
  CHECK(which == 3 && live_a == 0 && live_b == 0);
  return 0;
}
