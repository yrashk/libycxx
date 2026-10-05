// [expr.dynamic.cast]/8-/10: the result of a runtime-checked dynamic_cast depends on the most
// derived object v points to: void* gives the most derived object (/8); /9.1 a downcast to the
// unique C object derived from the subobject v points to; /9.2 a cast to the unambiguous public
// base C of the most derived object when v points to a public base class subobject of it; /9.3
// otherwise failure (null, or bad_cast for references, /10). The same cast expressions (fixed
// static source and target types) are applied over and over to objects of many dynamic types
// in many orders, so a runtime that remembers earlier results must key them on the dynamic
// type and on which subobject v points to.
// REQUIRES: exceptions
#include <typeinfo>
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
struct Pad {
  char pad[40] = {};
};
struct D1 : A, B {};
struct D2 : Pad, B, A {};
struct D3 : C, D1 {};
struct Amb : D1, D2 {};
struct PrivB : A, private B {};
struct ProtA : protected A, public B {
  A* asA() { return this; }
};
struct VD : virtual A, virtual B {};
struct VD2 : Pad, VD, virtual C {};
struct VD3 : virtual VD, C {};

[[gnu::noinline]] static B* to_B(A* p) { return dynamic_cast<B*>(p); }
[[gnu::noinline]] static D1* to_D1(A* p) { return dynamic_cast<D1*>(p); }
[[gnu::noinline]] static C* to_C(A* p) { return dynamic_cast<C*>(p); }
[[gnu::noinline]] static void* to_void(A* p) { return dynamic_cast<void*>(p); }
[[gnu::noinline]] static A* B_to_A(B* p) { return dynamic_cast<A*>(p); }
[[gnu::noinline]] static bool ref_to_B(A& r, B*& out) {
  try {
    out = &dynamic_cast<B&>(r);
    return true;
  } catch (const std::bad_cast&) {
    return false;
  }
}

struct Case {
  A* src;
  B* b;
  D1* d1;
  C* c;
  void* whole;
  B* bsrc;  // a B subobject (may be null)
  A* a_from_b;
};

static A a;
static D1 d1;
static D2 d2;
static D3 d3;
static Amb amb;
static PrivB privb;
static ProtA prota;
static VD vd;
static VD2 vd2;
static VD3 vd3;

int main() {
  A* amb_a1 = static_cast<A*>(static_cast<D1*>(&amb));
  A* amb_a2 = static_cast<A*>(static_cast<D2*>(&amb));
  B* amb_b1 = static_cast<B*>(static_cast<D1*>(&amb));
  const Case cases[] = {
      {&a, nullptr, nullptr, nullptr, &a, nullptr, nullptr},
      {&d1, &d1, &d1, nullptr, &d1, &d1, &d1},
      {&d2, &d2, nullptr, nullptr, &d2, &d2, &d2},
      {&d3, &d3, &d3, &d3, &d3, &d3, &d3},
      // /9.1: D1 is the unique D1 derived from the first A; B, A ambiguous in Amb
      {amb_a1, nullptr, static_cast<D1*>(&amb), nullptr, &amb, amb_b1, nullptr},
      // /9.2: the second A is a public base subobject of Amb, which has a unique public D1
      {amb_a2, nullptr, static_cast<D1*>(&amb), nullptr, &amb, nullptr, nullptr},
      {&privb, nullptr, nullptr, nullptr, &privb, nullptr, nullptr},
      // v points to a protected base subobject: neither /9.1 nor /9.2 applies
      {prota.asA(), nullptr, nullptr, nullptr, &prota, &prota, nullptr},
      {&vd, &vd, nullptr, nullptr, &vd, &vd, &vd},
      {&vd2, &vd2, nullptr, &vd2, &vd2, &vd2, &vd2},
      {&vd3, &vd3, nullptr, &vd3, &vd3, &vd3, &vd3},
  };
  constexpr unsigned n = sizeof(cases) / sizeof(cases[0]);
  auto check = [&](unsigned i) {
    const Case& k = cases[i];
    CHECK(to_B(k.src) == k.b);
    CHECK(to_D1(k.src) == k.d1);
    CHECK(to_C(k.src) == k.c);
    CHECK(to_void(k.src) == k.whole);
    B* r = nullptr;
    CHECK(ref_to_B(*k.src, r) == (k.b != nullptr));
    CHECK(r == k.b);
    if (k.bsrc) CHECK(B_to_A(k.bsrc) == k.a_from_b);
    if (k.b) CHECK(k.b->b == 2);
    if (k.d1) CHECK(k.d1->a == 1 && k.d1->b == 2);
  };
  CHECK(to_B(nullptr) == nullptr && to_void(nullptr) == nullptr);
  for (int round = 0; round < 3; ++round) {
    for (unsigned i = 0; i < n; ++i) check(i);
    for (unsigned i = n; i-- > 0;) check(i);
    for (unsigned i = 0; i < n; ++i) {
      check(i);
      check(i);
    }
  }
  unsigned x = 99;
  for (int k = 0; k < 5000; ++k) {
    x = x * 1664525u + 1013904223u;
    check((x >> 10) % n);
  }
  return 0;
}
