// [expr.dynamic.cast]/9: both checks require public bases: /9.1 "v points (refers) to a
// public base class subobject of a C object", /9.2 "v points (refers) to a public base class
// subobject of the most derived object, and the type of the most derived object has a base
// class, of type C, that is unambiguous and public". Protected derivation fails both.
// REQUIRES: exceptions
#include <typeinfo>
#include "check.hpp"

struct Base {
  virtual ~Base() = default;
};
struct Side {
  virtual ~Side() = default;
};
struct ProtBase : protected Base, public Side {
  Base* base() { return this; }
};
struct ProtSide : public Base, protected Side {
  Side* side() { return this; }
};
struct Outer : ProtBase {};

int main() {
  ProtBase pb;
  Base* b = pb.base();
  CHECK(dynamic_cast<ProtBase*>(b) == nullptr);  // downcast from a non-public base
  CHECK(dynamic_cast<Side*>(b) == nullptr);      // cross cast from a non-public base
  Side* s = &pb;
  CHECK(dynamic_cast<ProtBase*>(s) == &pb);
  CHECK(dynamic_cast<Base*>(s) == nullptr);  // target base is not public

  ProtSide ps;
  Base* b2 = &ps;
  CHECK(dynamic_cast<Side*>(b2) == nullptr);
  CHECK(dynamic_cast<ProtSide*>(b2) == &ps);
  CHECK(dynamic_cast<ProtSide*>(ps.side()) == nullptr);

  // the base is public in its immediate derived class but that class is a protected base
  Outer o;
  Side* os = &o;
  CHECK(dynamic_cast<Outer*>(os) == &o);
  CHECK(dynamic_cast<Base*>(os) == nullptr);

  bool threw = false;
  try {
    (void)dynamic_cast<ProtBase&>(*b);
  } catch (const std::bad_cast&) {
    threw = true;
  }
  CHECK(threw);
  return 0;
}
