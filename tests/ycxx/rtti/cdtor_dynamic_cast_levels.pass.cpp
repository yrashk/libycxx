// [class.cdtor]/5-/6 at every level of a hierarchy: while the constructor or destructor of
// class X runs (including functions it calls), typeid of the object yields X's type_info and
// for dynamic_cast "this object is considered to be a most derived object that has the type of
// the constructor or destructor's class" -- so casts to classes derived from X fail, casts to
// X's bases succeed, and dynamic_cast<void*> ([expr.dynamic.cast]/8) yields the X subobject.
// Objects of different most derived types are built and destroyed in turn, interleaved with
// the same casts on complete objects, so remembered results must take the construction state
// into account. The hierarchy has a virtual base and a sibling base placed before the chain,
// so the X subobject is not at the start of the complete object.
#include <typeinfo>
#include "check.hpp"

struct L0;
struct S;
struct L1;
struct L2;
struct L3;

static int errors = 0;
static int probes = 0;

static void probe(L0* p, int level, const void* self);
static void probe_s(S* p, const void* self);

struct L0 {
  int v0 = 10;
  L0() { probe(this, 0, this); }
  virtual ~L0() { probe(this, 0, this); }
};
struct S {
  int vs = 20;
  S() { probe_s(this, this); }
  virtual ~S() { probe_s(this, this); }
};
struct L1 : virtual L0 {
  int v1 = 11;
  L1() { probe(this, 1, this); }
  ~L1() override { probe(this, 1, this); }
};
struct L2 : S, L1 {
  int v2 = 12;
  L2() { probe(this, 2, this); }
  ~L2() override { probe(this, 2, this); }
};
struct L3 : L2 {
  int v3 = 13;
  L3() { probe(this, 3, this); }
  ~L3() override { probe(this, 3, this); }
};

static const std::type_info* const level_type[] = {&typeid(L0), &typeid(L1), &typeid(L2), &typeid(L3)};

static void probe(L0* p, int level, const void* self) {
  ++probes;
  if (typeid(*p) != *level_type[level]) ++errors;
  if (dynamic_cast<void*>(p) != self) ++errors;
  if ((dynamic_cast<L1*>(p) != nullptr) != (level >= 1)) ++errors;
  if ((dynamic_cast<L2*>(p) != nullptr) != (level >= 2)) ++errors;
  if ((dynamic_cast<L3*>(p) != nullptr) != (level >= 3)) ++errors;
  if ((dynamic_cast<S*>(p) != nullptr) != (level >= 2)) ++errors;  // cross cast to the sibling
  if (level >= 2 && dynamic_cast<S*>(p)->vs != 20) ++errors;
  if (level >= 1 && dynamic_cast<void*>(dynamic_cast<L1*>(p)) != self) ++errors;
}

static void probe_s(S* p, const void* self) {
  ++probes;
  if (typeid(*p) != typeid(S)) ++errors;
  if (dynamic_cast<void*>(p) != self) ++errors;
  if (dynamic_cast<L2*>(p) != nullptr) ++errors;
}

template <class T>
static void complete_checks(T& obj, int level) {
  L0* p = &obj;
  if (typeid(*p) != *level_type[level]) ++errors;
  if (dynamic_cast<void*>(p) != static_cast<void*>(&obj)) ++errors;
  if ((dynamic_cast<L2*>(p) != nullptr) != (level >= 2)) ++errors;
  if ((dynamic_cast<L3*>(p) != nullptr) != (level >= 3)) ++errors;
  if ((dynamic_cast<S*>(p) != nullptr) != (level >= 2)) ++errors;
}

int main() {
  for (int round = 0; round < 50; ++round) {
    {
      L3 x;
      complete_checks(x, 3);
      L1 y;
      complete_checks(y, 1);
      complete_checks(x, 3);
    }
    {
      L2 z;
      complete_checks(z, 2);
      L0 w;
      complete_checks(w, 0);
      L3* h = new L3;
      complete_checks(*h, 3);
      complete_checks(z, 2);
      delete static_cast<L0*>(h);
    }
  }
  // per round: L3 (5 ctor + 5 dtor probes), L1 (2+2), L2 (4+4), L0 (1+1), L3 (5+5)
  CHECK(probes == 50 * (10 + 4 + 8 + 2 + 10));
  CHECK(errors == 0);
  return 0;
}
