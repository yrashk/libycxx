// [expr.dynamic.cast]/9: "(9.1) If, in the most derived object pointed (referred) to by v, v
// points (refers) to a public base class subobject of a C object, and if only one object of
// type C is derived from the subobject pointed (referred) to by v, the result points (refers)
// to that C object. (9.2) Otherwise, if v points (refers) to a public base class subobject of
// the most derived object, and the type of the most derived object has a base class, of type C,
// that is unambiguous and public, the result points (refers) to the C subobject of the most
// derived object. (9.3) Otherwise, the runtime check fails." /10: a failed pointer cast gives
// null, a failed reference cast throws something matching std::bad_cast; /8: void* gives the
// most derived object.
// Public single-inheritance chains sit on top of classes with private, protected, virtual and
// multiple bases; many sources share the most derived object's address (offset 0) while
// reaching it only through a non-public base. [class.paths]/1: with a virtual base reachable
// by several paths, the access is that of the path giving most access. Every cast is repeated
// in several interleavings so that remembered results cannot leak between cases.
#include <typeinfo>
#include "check.hpp"

struct B {
  int b = 1;
  virtual ~B() = default;
};
struct X {
  int x = 2;
  virtual ~X() = default;
};

// 1. Public B inside a public chain that is itself a private base: In2 is found from its B
//    (9.1) although In2 is a private base of the most derived object; Top/Out are not.
struct In : B {};
struct In2 : In {};
struct Out : private In2 {
  B* b_of_out() { return this; }
  In2* in2_of_out() { return this; }
};
struct Top : Out {};
struct Top2a : Top {};

// Two such In2 subobjects, each reached from its own B.
struct Out2 : private In2 {
  B* b_of_out2() { return this; }
  In2* in2_of_out2() { return this; }
};
struct Both : Out, Out2 {};

// 2. Private X at offset 0, public B beside it; a public chain on top.
struct Y : private X, public B {
  X* x_of_y() { return this; }
};
struct Z : Y {};
struct Z2 : Z {};

// 3. Protected X at offset 0 under a public chain.
struct Yp : protected X {
  X* x_of_yp() { return this; }
};
struct Zp : Yp {};
struct Zp2 : Zp {};

// 4. A protected B inside a chain beside a public one.
struct Q : protected B {
  B* b_of_q() { return this; }
};
struct W : B {};
struct T : Q, W {};
struct T2 : T {};

// 5. A virtual B: private on one path, public on another (so public overall), chains on top.
struct VP : private virtual B {};
struct VQ : virtual B {};
struct VJ : VP, VQ {};
struct VK : VJ {};
struct VL : VK {};

// 6. A virtual B that is non-public on every path (protected on one, so that the most derived
//    class may still construct it).
struct VP1 : protected virtual B {
  B* b_of_vp1() { return this; }
};
struct VP2 : private virtual B {};
struct VM : VP1, VP2 {};
struct VN : VM {};

// 7. A public chain over a class with a public and a private base, source = public base.
struct M : X, private B {
  B* b_of_m() { return this; }
};
struct N : M {};
struct N2 : N {};

template <class To, class From>
bool ref_cast_throws(From& f) {
  try {
    (void)dynamic_cast<To&>(f);
  } catch (const std::bad_cast&) {
    return true;
  }
  return false;
}

static void case1() {
  Top2a t;
  B* b = t.b_of_out();
  CHECK(static_cast<void*>(b) == static_cast<void*>(&t));  // all at offset 0
  CHECK(dynamic_cast<In2*>(b) == t.in2_of_out());           // 9.1
  CHECK(dynamic_cast<In*>(b) == static_cast<In*>(t.in2_of_out()));
  CHECK(dynamic_cast<Top2a*>(b) == nullptr);
  CHECK(dynamic_cast<Top*>(b) == nullptr);
  CHECK(dynamic_cast<Out*>(b) == nullptr);
  CHECK(dynamic_cast<void*>(b) == static_cast<void*>(&t));
  CHECK(ref_cast_throws<Top2a>(*b));
  CHECK(ref_cast_throws<Out>(*b));
  CHECK(&dynamic_cast<In2&>(*b) == t.in2_of_out());
  // From the In2 pointer (a private base of the most derived object).
  In2* i2 = t.in2_of_out();
  CHECK(dynamic_cast<Top*>(i2) == nullptr);
  CHECK(dynamic_cast<B*>(i2) == b);  // an upcast (/4)
  // From a public base downwards: Out is public in Top2a.
  Out* o = &t;
  CHECK(dynamic_cast<Top2a*>(o) == &t);
  CHECK(dynamic_cast<Top*>(o) == &t);
}

static void case1_both() {
  Both bo;
  B* b1 = bo.b_of_out();
  B* b2 = bo.b_of_out2();
  CHECK(b1 != b2);
  CHECK(dynamic_cast<In2*>(b1) == bo.in2_of_out());
  CHECK(dynamic_cast<In2*>(b2) == bo.in2_of_out2());
  CHECK(dynamic_cast<Both*>(b1) == nullptr);
  CHECK(dynamic_cast<Both*>(b2) == nullptr);
  CHECK(dynamic_cast<Out*>(b1) == nullptr);
  CHECK(dynamic_cast<Out2*>(b2) == nullptr);
  CHECK(dynamic_cast<void*>(b1) == static_cast<void*>(&bo));
  CHECK(dynamic_cast<void*>(b2) == static_cast<void*>(&bo));
  Out2* o2 = &bo;
  CHECK(dynamic_cast<Both*>(o2) == &bo);
  CHECK(dynamic_cast<Out*>(o2) == static_cast<Out*>(&bo));  // cross cast, 9.2
}

static void case2() {
  Z2 z;
  X* x = z.x_of_y();
  CHECK(static_cast<void*>(x) == static_cast<void*>(&z));
  CHECK(dynamic_cast<Z2*>(x) == nullptr);
  CHECK(dynamic_cast<Z*>(x) == nullptr);
  CHECK(dynamic_cast<Y*>(x) == nullptr);
  CHECK(dynamic_cast<B*>(x) == nullptr);  // cross cast needs a public source
  CHECK(dynamic_cast<void*>(x) == static_cast<void*>(&z));
  CHECK(ref_cast_throws<Z2>(*x));
  CHECK(ref_cast_throws<B>(*x));
  B* b = &z;
  CHECK(dynamic_cast<Z2*>(b) == &z);
  CHECK(dynamic_cast<Y*>(b) == &z);
  CHECK(dynamic_cast<X*>(b) == nullptr);  // X is not a public base
  CHECK(ref_cast_throws<X>(*b));
  Z zz;  // a shorter chain, same layout
  CHECK(dynamic_cast<Z2*>(static_cast<B*>(&zz)) == nullptr);
  CHECK(dynamic_cast<Z*>(static_cast<B*>(&zz)) == &zz);
  CHECK(dynamic_cast<Z*>(zz.x_of_y()) == nullptr);
}

static void case3() {
  Zp2 z;
  X* x = z.x_of_yp();
  CHECK(static_cast<void*>(x) == static_cast<void*>(&z));
  CHECK(dynamic_cast<Zp2*>(x) == nullptr);
  CHECK(dynamic_cast<Zp*>(x) == nullptr);
  CHECK(dynamic_cast<Yp*>(x) == nullptr);
  CHECK(dynamic_cast<void*>(x) == static_cast<void*>(&z));
  CHECK(ref_cast_throws<Zp>(*x));
  Yp* yp = &z;
  CHECK(dynamic_cast<Zp2*>(yp) == &z);
  CHECK(dynamic_cast<Zp*>(yp) == &z);
}

static void case4() {
  T2 t;
  B* bq = t.b_of_q();  // protected B inside Q
  B* bw = static_cast<W*>(&t);  // public B inside W
  CHECK(bq != bw);
  CHECK(dynamic_cast<T2*>(bq) == nullptr);
  CHECK(dynamic_cast<T*>(bq) == nullptr);
  CHECK(dynamic_cast<Q*>(bq) == nullptr);
  CHECK(dynamic_cast<W*>(bq) == nullptr);
  CHECK(dynamic_cast<T2*>(bw) == &t);
  CHECK(dynamic_cast<T*>(bw) == &t);
  CHECK(dynamic_cast<W*>(bw) == static_cast<W*>(&t));
  CHECK(dynamic_cast<Q*>(bw) == static_cast<Q*>(&t));  // 9.2: Q is a public base of T2
  CHECK(dynamic_cast<void*>(bq) == static_cast<void*>(&t));
  CHECK(ref_cast_throws<T2>(*bq));
}

static void case5() {
  VL l;
  B* b = static_cast<VQ*>(&l);
  CHECK(dynamic_cast<VL*>(b) == &l);
  CHECK(dynamic_cast<VK*>(b) == &l);
  CHECK(dynamic_cast<VJ*>(b) == &l);
  CHECK(dynamic_cast<VQ*>(b) == static_cast<VQ*>(&l));
  // B is not a public base of VP, so not 9.1; but B is a public base of the most derived
  // object (via VQ) and VP is an unambiguous public base of it: 9.2.
  CHECK(dynamic_cast<VP*>(b) == static_cast<VP*>(&l));
  CHECK(dynamic_cast<void*>(b) == static_cast<void*>(&l));
  VJ j;  // the bottom of the chain as the most derived object
  B* bj = static_cast<VQ*>(&j);
  CHECK(dynamic_cast<VL*>(bj) == nullptr);
  CHECK(dynamic_cast<VJ*>(bj) == &j);
  CHECK(dynamic_cast<VP*>(bj) == static_cast<VP*>(&j));
}

static void case6() {
  VN n;
  B* b = static_cast<VP1*>(&n)->b_of_vp1();
  CHECK(dynamic_cast<VN*>(b) == nullptr);
  CHECK(dynamic_cast<VM*>(b) == nullptr);
  CHECK(dynamic_cast<VP1*>(b) == nullptr);
  CHECK(dynamic_cast<VP2*>(b) == nullptr);
  CHECK(dynamic_cast<void*>(b) == static_cast<void*>(&n));
  CHECK(ref_cast_throws<VN>(*b));
}

static void case7() {
  N2 n;
  X* x = &n;
  CHECK(static_cast<void*>(x) == static_cast<void*>(&n));
  CHECK(dynamic_cast<N2*>(x) == &n);
  CHECK(dynamic_cast<M*>(x) == &n);
  CHECK(dynamic_cast<B*>(x) == nullptr);  // B is private
  B* b = n.b_of_m();
  CHECK(dynamic_cast<N2*>(b) == nullptr);
  CHECK(dynamic_cast<X*>(b) == nullptr);
  CHECK(dynamic_cast<M*>(b) == nullptr);
  N nn;
  CHECK(dynamic_cast<N2*>(static_cast<X*>(&nn)) == nullptr);
  CHECK(dynamic_cast<N*>(static_cast<X*>(&nn)) == &nn);
}

int main() {
  void (*cases[])() = {case1, case1_both, case2, case3, case4, case5, case6, case7};
  constexpr int n = sizeof cases / sizeof cases[0];
  for (int round = 0; round < 6; ++round)
    for (int i = 0; i < n; ++i) cases[(i * (round + 1) + round) % n]();  // varying orders
  for (int i = n - 1; i >= 0; --i) cases[i]();
}
