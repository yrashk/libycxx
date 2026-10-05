// A virtual base V reached from the most derived object through two paths whose accesses are
// public, protected or private, in both orders, below a public single-inheritance chain.
// [class.paths]/1: "If a name can be reached by several paths through a multiple inheritance
// graph, the access is that of the path that gives most access." So V is a public base of the
// most derived object F exactly when is_convertible_v<F*, V*> (one path is public), which is
// the oracle here for:
//  - [expr.dynamic.cast]/9: from the V subobject, a downcast to F (9.1: V a public base of an F
//    object) succeeds iff V is a public base of F; a cast to a middle class P succeeds iff V is
//    a public base of P (9.1) or of F (9.2, P being an unambiguous public base of F); a cross
//    cast between the two middle classes always succeeds (9.2: the source is a public base
//    subobject of F);
//    and inside V (V : VL0, VR0) the cross cast VL0 -> VR0 succeeds iff V is a public base of
//    F, while the downcast VL0 -> V always succeeds (9.1);
//  - [except.handle]/3.2: a handler for V matches a thrown F iff V is an unambiguous public base
//    of F (V is virtual, so never ambiguous).
// (Orders where the first path is private and a later one public, and all-private
// combinations, are left out: the latter is ill-formed, the former is rejected by Clang.)
#include <type_traits>
#include <typeinfo>
#include "check.hpp"

struct VL0 {
  int l0 = 2;
  virtual ~VL0() = default;
};
struct VR0 {
  int r0 = 3;
  virtual ~VR0() = default;
};
struct V : VL0, VR0 {
  int v = 1;
};

#define MIDDLE(Name, Access)            \
  struct Name : Access virtual V {      \
    V* vp() { return this; }            \
  }

MIDDLE(Pa, public);
MIDDLE(Pb, protected);
MIDDLE(Pc, private);
MIDDLE(Qa, public);
MIDDLE(Qb, protected);
MIDDLE(Qc, private);

template <class P, class Q>
struct D : P, Q {};
template <class P, class Q>
struct E : D<P, Q> {};
template <class P, class Q>
struct F : E<P, Q> {};

template <class Mid, class Fx>
V* v_of(Fx& f) {
  return static_cast<Mid&>(f).vp();
}

template <class P, class Q>
void one() {
  using FT = F<P, Q>;
  constexpr bool v_public = std::is_convertible_v<FT*, V*>;
  constexpr bool v_public_in_p = std::is_convertible_v<P*, V*>;
  constexpr bool v_public_in_q = std::is_convertible_v<Q*, V*>;
  static_assert(v_public == (v_public_in_p || v_public_in_q));
  FT f;
  V* v = v_of<P>(f);
  CHECK(v == v_of<Q>(f));  // one V
  CHECK(dynamic_cast<void*>(v) == static_cast<void*>(&f));
  CHECK((dynamic_cast<FT*>(v) == &f) == v_public);
  CHECK((dynamic_cast<D<P, Q>*>(v) == &f) == v_public);
  CHECK((dynamic_cast<P*>(v) == static_cast<P*>(&f)) == (v_public_in_p || v_public));
  CHECK((dynamic_cast<Q*>(v) == static_cast<Q*>(&f)) == (v_public_in_q || v_public));
  if (!(dynamic_cast<P*>(v) == static_cast<P*>(&f)) && !(v_public_in_p || v_public)) CHECK(dynamic_cast<P*>(v) == nullptr);
  // inside V: a downcast from VL0 to V is 9.1 (VL0 is a public base of the V object) and
  // always succeeds; the cross cast VL0 -> VR0 needs 9.2, so V public in F
  VL0* l0 = v;
  CHECK(dynamic_cast<V*>(l0) == v);
  CHECK((dynamic_cast<VR0*>(l0) == static_cast<VR0*>(v)) == v_public);
  CHECK((dynamic_cast<FT*>(l0) == &f) == v_public);
  P* p = &f;
  Q* q = &f;
  CHECK(dynamic_cast<Q*>(p) == q);
  CHECK(dynamic_cast<P*>(q) == p);
  CHECK(dynamic_cast<FT*>(p) == &f);
  bool threw = false;
  try {
    (void)dynamic_cast<FT&>(*v);
  } catch (const std::bad_cast&) {
    threw = true;
  }
  CHECK(threw == !v_public);
  bool caught = false;
  try {
    throw FT();
  } catch (const V&) {
    caught = true;
  } catch (...) {
  }
  CHECK(caught == v_public);
  caught = false;
  try {
    throw &f;
  } catch (V* pv) {
    caught = pv == v;
  } catch (...) {
  }
  CHECK(caught == v_public);
}

int main() {
  for (int round = 0; round < 2; ++round) {
    one<Pa, Qa>();
    one<Pa, Qb>();
    one<Pa, Qc>();
    one<Pb, Qa>();
    one<Pb, Qb>();
    one<Pb, Qc>();
    one<Qb, Pc>();
    one<Qa, Pb>();
    one<Qb, Pa>();
    one<Qa, Pc>();
  }
}
