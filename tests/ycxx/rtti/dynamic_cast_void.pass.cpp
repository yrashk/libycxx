// [expr.dynamic.cast]/8: "If T is 'pointer to cv void', then the result is a pointer to the
// most derived object pointed to by v." /6: null in, null out. /1: dynamic_cast "shall not
// cast away constness".
#include "check.hpp"

struct A {
  long a = 1;
  virtual ~A() = default;
};
struct B {
  long b = 2;
  virtual ~B() = default;
};
struct V {
  long v = 3;
  virtual ~V() = default;
};
struct M : A, B, virtual V {
  long m = 4;
};

int main() {
  M m;
  A* ap = &m;
  B* bp = &m;
  V* vp = &m;
  CHECK(static_cast<void*>(bp) != static_cast<void*>(&m));  // B is not at offset 0
  CHECK(dynamic_cast<void*>(ap) == static_cast<void*>(&m));
  CHECK(dynamic_cast<void*>(bp) == static_cast<void*>(&m));
  CHECK(dynamic_cast<void*>(vp) == static_cast<void*>(&m));

  const B* cbp = bp;
  const void* cv = dynamic_cast<const void*>(cbp);
  CHECK(cv == static_cast<const void*>(&m));
  const volatile void* cvv = dynamic_cast<const volatile void*>(cbp);
  CHECK(cvv == static_cast<const volatile void*>(&m));

  CHECK(dynamic_cast<void*>(static_cast<B*>(nullptr)) == nullptr);

  // a sub-object whose most derived object is itself
  B lone;
  CHECK(dynamic_cast<void*>(&lone) == &lone);
  return 0;
}
