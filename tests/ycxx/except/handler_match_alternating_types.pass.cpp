// [except.handle]/3: whether a handler matches depends only on the type E of the exception
// object and the handler's type; /3.2 a handler of type cv T or cv T& matches if "T is an
// unambiguous public base class of E", and the handler's reference/copy then refers to the T
// subobject of the exception object. The same try-block is entered over and over with
// exception objects of different types, in many interleavings, so that a runtime which
// remembers earlier match results must key them on the thrown type: a base reached at a
// different offset, an ambiguous base, a private base and an unrelated type all go through the
// same handler list.
#include "check.hpp"

struct A {
  int a = 100;
  virtual int who() const { return 0; }
  virtual ~A() = default;
};
struct B {
  int b = 200;
  virtual int who_b() const { return 0; }
  virtual ~B() = default;
};
struct Pad {
  long pad[3] = {7, 8, 9};
};
// A at offset 0, B after it
struct D1 : A, B {
  D1() { a = 1; b = 11; }
  int who() const override { return 1; }
  int who_b() const override { return 1; }
};
// B first, A at a nonzero offset (and further away still in D2b)
struct D2 : B, A {
  D2() { a = 2; b = 12; }
  int who() const override { return 2; }
  int who_b() const override { return 2; }
};
struct D2b : Pad, D2 {
  D2b() { a = 3; b = 13; }
  int who() const override { return 3; }
  int who_b() const override { return 3; }
};
// two A and two B subobjects: both ambiguous
struct Amb : D1, D2 {};
// A private, B public
struct PrivA : private A, public B {
  PrivA() { b = 14; }
  int who_b() const override { return 4; }
};
// one A shared through virtual inheritance: unambiguous
struct VL : virtual A {};
struct VR : virtual A {};
struct VD : VL, VR, B {
  VD() { a = 5; b = 15; }
  int who() const override { return 5; }
  int who_b() const override { return 5; }
};
// virtual and non-virtual A: ambiguous
struct AN : A {};
struct Mix : VL, AN {};
struct Other {
  int o = 6;
};

enum Kind { none, gotA, gotB, gotAval, gotInt, gotOther, gotAll };
struct Result {
  Kind k = none;
  int tagA = -1, tagB = -1, whoA = -1, whoB = -1;
};

template <class E>
static void raise() {
  throw E();
}

// handlers in the order A&, B&, int, Other&, ...
static Result by_ref(void (*f)()) {
  Result r;
  try {
    f();
  } catch (const A& x) {
    r.k = gotA;
    r.tagA = x.a;
    r.whoA = x.who();
  } catch (B& x) {
    r.k = gotB;
    r.tagB = x.b;
    r.whoB = x.who_b();
  } catch (int) {
    r.k = gotInt;
  } catch (Other& o) {
    r.k = gotOther;
    r.tagA = o.o;
  } catch (...) {
    r.k = gotAll;
  }
  return r;
}

// B before A, and A caught by value (a sliced copy: who() is A's)
static Result by_val(void (*f)()) {
  Result r;
  try {
    f();
  } catch (volatile B& x) {
    r.k = gotB;
    r.tagB = const_cast<B&>(x).b;
    r.whoB = const_cast<B&>(x).who_b();
  } catch (A x) {
    r.k = gotAval;
    r.tagA = x.a;
    r.whoA = x.who();
  } catch (...) {
    r.k = gotAll;
  }
  return r;
}

struct Case {
  void (*f)();
  Result ref, val;
};

static void throw_int() { throw 5; }
static void throw_long() { throw 5L; }

static bool same(const Result& x, const Result& y) {
  return x.k == y.k && x.tagA == y.tagA && x.tagB == y.tagB && x.whoA == y.whoA && x.whoB == y.whoB;
}

int main() {
  const Case cases[] = {
      {raise<A>, {gotA, 100, -1, 0, -1}, {gotAval, 100, -1, 0, -1}},
      {raise<B>, {gotB, -1, 200, -1, 0}, {gotB, -1, 200, -1, 0}},
      {raise<D1>, {gotA, 1, -1, 1, -1}, {gotB, -1, 11, -1, 1}},
      {raise<D2>, {gotA, 2, -1, 2, -1}, {gotB, -1, 12, -1, 2}},
      {raise<D2b>, {gotA, 3, -1, 3, -1}, {gotB, -1, 13, -1, 3}},
      {raise<Amb>, {gotAll, -1, -1, -1, -1}, {gotAll, -1, -1, -1, -1}},
      {raise<PrivA>, {gotB, -1, 14, -1, 4}, {gotB, -1, 14, -1, 4}},
      {raise<VD>, {gotA, 5, -1, 5, -1}, {gotB, -1, 15, -1, 5}},
      {raise<VL>, {gotA, 100, -1, 0, -1}, {gotAval, 100, -1, 0, -1}},
      {raise<Mix>, {gotAll, -1, -1, -1, -1}, {gotAll, -1, -1, -1, -1}},
      {raise<Other>, {gotOther, 6, -1, -1, -1}, {gotAll, -1, -1, -1, -1}},
      {throw_int, {gotInt, -1, -1, -1, -1}, {gotAll, -1, -1, -1, -1}},
      {throw_long, {gotAll, -1, -1, -1, -1}, {gotAll, -1, -1, -1, -1}},
  };
  constexpr unsigned n = sizeof(cases) / sizeof(cases[0]);

  // forward, backward, each one repeatedly, and a pseudo-random interleaving
  for (int round = 0; round < 3; ++round) {
    for (unsigned i = 0; i < n; ++i) {
      CHECK(same(by_ref(cases[i].f), cases[i].ref));
      CHECK(same(by_val(cases[i].f), cases[i].val));
    }
    for (unsigned i = n; i-- > 0;) {
      CHECK(same(by_val(cases[i].f), cases[i].val));
      CHECK(same(by_ref(cases[i].f), cases[i].ref));
    }
  }
  for (unsigned i = 0; i < n; ++i)
    for (int k = 0; k < 4; ++k) CHECK(same(by_ref(cases[i].f), cases[i].ref));
  unsigned x = 12345;
  for (int k = 0; k < 2000; ++k) {
    x = x * 1103515245u + 12345u;
    const Case& c = cases[(x >> 16) % n];
    if (x & 0x100)
      CHECK(same(by_ref(c.f), c.ref));
    else
      CHECK(same(by_val(c.f), c.val));
  }
  return 0;
}
