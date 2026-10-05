// [except.handle]/3.3: pointer handlers match via "a standard pointer conversion" (here:
// object pointer to cv void*, [conv.ptr]/2) and/or "a qualification conversion". The
// qualification of the pointee must not be dropped: const int* does not convert to void*.
// Function pointers and pointers to members do not convert to void*.
// REQUIRES: exceptions
#include "check.hpp"

struct S {
  int m;
  virtual ~S() = default;
};
struct T {
  int t;
};
struct ST : T, S {};
void fn() {}

int main() {
  int i = 0;
  int which = 0;
  try {
    throw &i;
  } catch (void* p) {
    which = p == &i ? 1 : -1;
  }
  CHECK(which == 1);

  which = 0;
  try {
    throw &i;
  } catch (const volatile void* p) {
    which = p == &i ? 1 : -1;
  }
  CHECK(which == 1);

  const int ci = 0;
  which = 0;
  try {
    try {
      throw &ci;
    } catch (void*) {
      which = -1;
    }
  } catch (const void* p) {
    which = p == &ci ? 1 : -2;
  }
  CHECK(which == 1);

  // class pointer to void*: the value is that of the thrown pointer (no dynamic adjustment)
  ST st;
  S* sp = &st;
  which = 0;
  try {
    throw sp;
  } catch (void* p) {
    which = p == static_cast<void*>(sp) ? 1 : -1;
  }
  CHECK(which == 1);

  // function pointer: not a match for void*
  which = 0;
  try {
    try {
      throw &fn;
    } catch (void*) {
      which = -1;
    } catch (const void*) {
      which = -2;
    }
  } catch (void (*f)()) {
    which = f == &fn ? 1 : -3;
  }
  CHECK(which == 1);

  // pointer to member: not a match for void*
  which = 0;
  try {
    try {
      throw &T::t;
    } catch (void*) {
      which = -1;
    }
  } catch (int T::*pm) {
    which = pm == &T::t ? 1 : -2;
  }
  CHECK(which == 1);

  // void* does not convert back to int*
  which = 0;
  try {
    try {
      throw static_cast<void*>(&i);
    } catch (int*) {
      which = -1;
    }
  } catch (void*) {
    which = 1;
  }
  CHECK(which == 1);
  return 0;
}
