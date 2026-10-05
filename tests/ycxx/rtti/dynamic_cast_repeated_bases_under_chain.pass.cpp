// [expr.dynamic.cast]/9.1: the result is "that C object" when v points to a public base class
// subobject of a C object "and if only one object of type C is derived from the subobject
// pointed (referred) to by v"; 9.2: otherwise a cast to C succeeds when v points to a public
// base class subobject of the most derived object and C is an unambiguous public base of it;
// 9.3: otherwise it fails. A public single-inheritance chain sits on top of a class in which
// the source's type occurs more than once:
//  - non-virtually (two B subobjects, one at offset 0): a downcast to the chain from either B
//    succeeds (only one chain object is derived from each), and so do cross casts (9.2);
//  - a virtual base shared by two C subobjects: a cast from it to C fails (two C objects are
//    derived from it, and C is ambiguous in the most derived object), while casts to the
//    chain, to C1/C2 and cross casts between the C subobjects succeed.
// REQUIRES: exceptions
#include <typeinfo>
#include "check.hpp"

struct B {
  int b = 0;
  virtual ~B() = default;
};
struct L : B {
  int l = 1;
};
struct R : B {
  int r = 2;
};
struct LR : L, R {};
struct T1 : LR {};
struct T2 : T1 {};

struct V {
  int v = 3;
  virtual ~V() = default;
};
struct C : virtual V {
  int c = 4;
};
struct C1 : C {};
struct C2 : C {};
struct D : C1, C2 {};
struct E : D {};
struct G : E {};

template <class To, class From>
bool ref_fails(From& f) {
  try {
    (void)dynamic_cast<To&>(f);
  } catch (const std::bad_cast&) {
    return true;
  }
  return false;
}

static void repeated_nonvirtual() {
  T2 t;
  B* bl = static_cast<L*>(&t);  // offset 0
  B* br = static_cast<R*>(&t);
  CHECK(static_cast<void*>(bl) == static_cast<void*>(&t));
  CHECK(bl != br);
  CHECK(dynamic_cast<T2*>(bl) == &t);
  CHECK(dynamic_cast<T1*>(bl) == &t);
  CHECK(dynamic_cast<LR*>(bl) == &t);
  CHECK(dynamic_cast<T2*>(br) == &t);
  CHECK(dynamic_cast<LR*>(br) == &t);
  CHECK(dynamic_cast<L*>(bl) == static_cast<L*>(&t));
  CHECK(dynamic_cast<R*>(br) == static_cast<R*>(&t));
  CHECK(dynamic_cast<R*>(bl) == static_cast<R*>(&t));  // cross cast, 9.2
  CHECK(dynamic_cast<L*>(br) == static_cast<L*>(&t));
  CHECK(dynamic_cast<void*>(br) == static_cast<void*>(&t));
  CHECK(&dynamic_cast<T2&>(*br) == &t);
  // the same classes as most derived objects lower in the chain
  T1 t1;
  CHECK(dynamic_cast<T2*>(static_cast<B*>(static_cast<L*>(&t1))) == nullptr);
  CHECK(dynamic_cast<T1*>(static_cast<B*>(static_cast<R*>(&t1))) == &t1);
  LR lr;
  CHECK(dynamic_cast<T1*>(static_cast<B*>(static_cast<L*>(&lr))) == nullptr);
  CHECK(dynamic_cast<R*>(static_cast<B*>(static_cast<L*>(&lr))) == static_cast<R*>(&lr));
  L l;
  CHECK(dynamic_cast<R*>(static_cast<B*>(&l)) == nullptr);
  CHECK(dynamic_cast<LR*>(static_cast<B*>(&l)) == nullptr);
}

static void shared_virtual() {
  G g;
  V* v = &g;  // the single V
  CHECK(dynamic_cast<C*>(v) == nullptr);  // two C objects derive from it; C ambiguous in G
  CHECK(ref_fails<C>(*v));
  CHECK(dynamic_cast<C1*>(v) == static_cast<C1*>(&g));
  CHECK(dynamic_cast<C2*>(v) == static_cast<C2*>(&g));
  CHECK(dynamic_cast<D*>(v) == &g);
  CHECK(dynamic_cast<E*>(v) == &g);
  CHECK(dynamic_cast<G*>(v) == &g);
  CHECK(dynamic_cast<void*>(v) == static_cast<void*>(&g));
  C* c1 = static_cast<C1*>(&g);
  C* c2 = static_cast<C2*>(&g);
  CHECK(dynamic_cast<C2*>(c1) == static_cast<C2*>(&g));  // cross cast, 9.2
  CHECK(dynamic_cast<C1*>(c2) == static_cast<C1*>(&g));
  CHECK(dynamic_cast<C1*>(c1) == static_cast<C1*>(&g));  // 9.1
  CHECK(dynamic_cast<G*>(c2) == &g);
  CHECK(dynamic_cast<V*>(c2) == v);  // an upcast
  // a most derived D (shorter chain): still ambiguous; E/G are not there
  D d;
  V* dv = &d;
  CHECK(dynamic_cast<C*>(dv) == nullptr);
  CHECK(dynamic_cast<G*>(dv) == nullptr);
  CHECK(dynamic_cast<D*>(dv) == &d);
  // a C1 alone: one C, so the cast from V succeeds
  C1 one;
  V* ov = &one;
  CHECK(dynamic_cast<C*>(ov) == static_cast<C*>(&one));
  CHECK(dynamic_cast<C2*>(ov) == nullptr);
}

int main() {
  for (int i = 0; i < 4; ++i) {
    if (i % 2) {
      shared_virtual();
      repeated_nonvirtual();
    } else {
      repeated_nonvirtual();
      shared_virtual();
    }
  }
}
