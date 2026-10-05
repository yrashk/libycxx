// [expr.dynamic.cast]/9: a source subobject reached only through a private base is neither a
// public base of a dst object (9.1) nor a public base of the most derived object (9.2), so the
// cast fails, even when dst sits in a public single-inheritance chain at the most derived
// object's address. Also with a protected base and a
// private base beside another.
#include <cassert>
#include <typeinfo>

struct B {
  virtual ~B() = default;
};

// C : private B; D : C (single public chain above C); E : D.
struct C : private B {
  B* as_b() { return this; }
};
struct D : C {};
struct E : D {};

struct C2 : protected B {
  B* as_b() { return this; }
};
struct D2 : C2 {};

struct X {
  virtual ~X() = default;
};
struct C3 : X, private B {
  B* as_b() { return this; }
};
struct D3 : C3 {};

struct P : B {}; // the public counterpart: the cast succeeds
struct Q : P {};

int main() {
  E e;
  B* b = e.as_b();
  assert(dynamic_cast<E*>(b) == nullptr);
  assert(dynamic_cast<D*>(b) == nullptr);
  assert(dynamic_cast<C*>(b) == nullptr);
  void* mdo = dynamic_cast<void*>(b);
  assert(mdo == static_cast<void*>(&e));

  D d;
  assert(dynamic_cast<D*>(d.as_b()) == nullptr);

  D2 d2;
  assert(dynamic_cast<D2*>(d2.as_b()) == nullptr);
  assert(dynamic_cast<C2*>(d2.as_b()) == nullptr);

  D3 d3;
  assert(dynamic_cast<D3*>(d3.as_b()) == nullptr);
  assert(dynamic_cast<X*>(d3.as_b()) == nullptr);

  Q q;
  B* qb = &q;
  assert(dynamic_cast<Q*>(qb) == &q);
  assert(dynamic_cast<P*>(qb) == &q);
  bool threw = false;
  try {
    (void)dynamic_cast<E&>(*b);
  } catch (const std::bad_cast&) {
    threw = true;
  }
  assert(threw);
}
