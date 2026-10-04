// [except.ctor]/2: "If an exception is thrown during the destruction of temporaries or local
// variables for a return statement, the destructor for the returned object (if any) is also
// invoked." Example 1: at #1 the returned A is constructed, b destroyed, y's destructor
// throws, then the returned object is destroyed, followed by a.
#include "check.hpp"

static int log_[16];
static int n = 0;

struct A {
  int id;
  A(int i) : id(i) {}
  ~A() { log_[n++] = id; }
};
struct Y {
  ~Y() noexcept(false) { throw 0; }
};

A f() {
  try {
    A a(1);
    Y y;
    A b(2);
    return {3};  // #1
  } catch (...) {
  }
  return {4};  // #2
}

int main() {
  {
    A r = f();
    CHECK(r.id == 4);
    CHECK(n == 3);
    CHECK(log_[0] == 2);  // b
    CHECK(log_[1] == 3);  // the returned object
    CHECK(log_[2] == 1);  // a
  }
  CHECK(n == 4 && log_[3] == 4);
  return 0;
}
