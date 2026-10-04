// [expr.dynamic.cast]/9.3 "Otherwise, the runtime check fails." Example 2:
//   class A { virtual void f(); }; class B { virtual void g(); };
//   class D : public virtual A, private B { };
//   D d; B* bp = (B*)&d; A* ap = &d;
//   D& dr = dynamic_cast<D&>(*bp);    // fails
//   ap = dynamic_cast<A*>(bp);        // fails
//   bp = dynamic_cast<B*>(ap);        // fails
//   ap = dynamic_cast<A*>(&d);        // succeeds
//   class E : public D, public B { }; class F : public E, public D { };
//   F f; A* ap = &f;                  // succeeds: finds unique A
//   D* dp = dynamic_cast<D*>(ap);     // fails: yields null; f has two D subobjects
//   E* ep1 = dynamic_cast<E*>(ap);    // succeeds
// (Compilers warn that the direct bases B of E and D of F are inaccessible due to ambiguity.)
#include <typeinfo>
#include "check.hpp"

class A {
  virtual void f() {}

 public:
  virtual ~A() = default;
};
class B {
  virtual void g() {}

 public:
  virtual ~B() = default;
};
class D : public virtual A, private B {};
class E : public D, public B {};
class F : public E, public D {};

int main() {
  D d;
  B* bp = (B*)&d;
  A* ap = &d;
  bool threw = false;
  try {
    (void)dynamic_cast<D&>(*bp);
  } catch (const std::bad_cast&) {
    threw = true;
  }
  CHECK(threw);
  CHECK(dynamic_cast<A*>(bp) == nullptr);
  CHECK(dynamic_cast<B*>(ap) == nullptr);
  CHECK(dynamic_cast<A*>(&d) == ap);
  CHECK(dynamic_cast<D*>(bp) == nullptr);

  F f;
  A* fap = &f;
  CHECK(fap != nullptr);
  CHECK(dynamic_cast<D*>(fap) == nullptr);
  E* ep1 = dynamic_cast<E*>(fap);
  CHECK(ep1 == static_cast<E*>(&f));
  CHECK(dynamic_cast<F*>(fap) == &f);
  // F has three B subobjects (a private one in each D, a public one directly in E): a cast to
  // B from the unique A fails because the most derived object's B is ambiguous (/9.2).
  CHECK(dynamic_cast<B*>(fap) == nullptr);
  return 0;
}
