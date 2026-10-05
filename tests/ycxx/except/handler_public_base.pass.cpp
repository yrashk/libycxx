// [except.handle]/3.2: "the handler is of type cv T or cv T& and T is an unambiguous public
// base class of E". /15.1: "if T is a base class of E, the variable is copy-initialized from
// an lvalue of type T designating the corresponding base class subobject of the exception
// object". Covers indirect bases, non-polymorphic classes, virtual bases and multiple
// inheritance (the reference binds to the right subobject).
// REQUIRES: exceptions
#include "check.hpp"

struct A {
  int a = 1;
};
struct B : A {
  int b = 2;
};
struct C : B {
  int c = 3;
};

struct L {
  int l = 10;
};
struct R {
  int r = 20;
};
struct LR : L, R {
  int lr = 30;
};

struct V {
  int v = 100;
  virtual ~V() = default;
};
struct V1 : virtual V {};
struct V2 : virtual V {};
struct VD : V1, V2 {};

int main() {
  int which = 0;
  try {
    throw C();
  } catch (A& a) {
    which = a.a;
  }
  CHECK(which == 1);

  which = 0;
  try {
    throw C();
  } catch (B b) {  // by value: sliced copy of the B subobject
    which = b.a + b.b;
  }
  CHECK(which == 3);

  // second base: the reference refers to the R subobject of the exception object
  which = 0;
  try {
    throw LR();
  } catch (R& r) {
    which = r.r;
    LR& full = static_cast<LR&>(r);
    CHECK(full.lr == 30 && full.l == 10);
  }
  CHECK(which == 20);

  which = 0;
  try {
    throw LR();
  } catch (const L& l) {
    which = l.l;
  }
  CHECK(which == 10);

  // virtual base shared through a diamond is unambiguous
  which = 0;
  try {
    throw VD();
  } catch (V& v) {
    which = v.v;
    CHECK(dynamic_cast<VD*>(&v) != nullptr);
  }
  CHECK(which == 100);

  which = 0;
  try {
    throw VD();
  } catch (V2& v2) {
    which = v2.v;
  }
  CHECK(which == 100);

  // a handler for a derived class does not match a base exception object
  which = 0;
  try {
    try {
      throw A();
    } catch (B&) {
      which = -1;
    }
  } catch (A&) {
    which = 1;
  }
  CHECK(which == 1);
  return 0;
}
