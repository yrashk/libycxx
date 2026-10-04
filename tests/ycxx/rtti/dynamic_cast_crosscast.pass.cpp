// [expr.dynamic.cast]/9.2: "Otherwise, if v points (refers) to a public base class subobject
// of the most derived object, and the type of the most derived object has a base class, of
// type C, that is unambiguous and public, the result points (refers) to the C subobject of
// the most derived object." This allows casting sideways between unrelated bases.
#include "check.hpp"

struct A {
  int a = 1;
  virtual ~A() = default;
};
struct B {
  int b = 2;
  virtual ~B() = default;
};
struct C {
  int c = 3;
  virtual ~C() = default;
};
struct AB : A, B {};
struct ABC : AB, C {};
struct NonPoly {
  int n = 4;
};
struct WithNP : A, NonPoly {};

int main() {
  AB ab;
  A* ap = &ab;
  B* bp = dynamic_cast<B*>(ap);
  CHECK(bp == static_cast<B*>(&ab));
  CHECK(bp->b == 2);
  CHECK(dynamic_cast<A*>(bp) == ap);
  CHECK(dynamic_cast<C*>(ap) == nullptr);

  ABC abc;
  C* cp = &abc;
  CHECK(dynamic_cast<A*>(cp) == static_cast<A*>(&abc));
  CHECK(dynamic_cast<B*>(cp) == static_cast<B*>(&abc));
  CHECK(dynamic_cast<AB*>(cp) == static_cast<AB*>(&abc));
  B* bp2 = &abc;
  CHECK(dynamic_cast<C*>(bp2) == cp);
  CHECK(dynamic_cast<ABC*>(bp2) == &abc);

  // the target need not be polymorphic
  WithNP w;
  A* wa = &w;
  NonPoly* np = dynamic_cast<NonPoly*>(wa);
  CHECK(np == static_cast<NonPoly*>(&w) && np->n == 4);

  // reference cross cast
  B& br = dynamic_cast<B&>(*ap);
  CHECK(&br == static_cast<B*>(&ab));
  return 0;
}
