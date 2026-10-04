// [expr.typeid]/4: "When typeid is applied to a glvalue whose type is a polymorphic class
// type, the result refers to a std::type_info object representing the type of the most
// derived object (that is, the dynamic type) to which the glvalue refers." /5: otherwise the
// static type, and "The expression is an unevaluated operand." /1: the result is an lvalue of
// static type const std::type_info whose lifetime extends to the end of the program.
#include <typeinfo>
#include <type_traits>
#include "check.hpp"

struct Poly {
  virtual ~Poly() = default;
};
struct Mid : Poly {};
struct Leaf : Mid {};
struct Other {
  virtual ~Other() = default;
};
struct Multi : Other, Leaf {};
struct V {
  virtual ~V() = default;
};
struct VD : virtual V {};

struct Plain {};
struct PlainDerived : Plain {};

static int evaluations = 0;
static Poly& counted(Poly& p) {
  ++evaluations;
  return p;
}
static Plain& counted_plain(Plain& p) {
  ++evaluations;
  return p;
}

static_assert(std::is_same_v<decltype(typeid(int)), const std::type_info&>);

int main() {
  Leaf leaf;
  Poly& pr = leaf;
  CHECK(typeid(pr) == typeid(Leaf));
  CHECK(typeid(pr) != typeid(Poly));
  Poly* pp = &leaf;
  CHECK(typeid(*pp) == typeid(Leaf));
  CHECK(typeid(pp) == typeid(Poly*));  // pointer: static type

  Multi m;
  Other& o = m;
  Poly& mp = m;
  CHECK(typeid(o) == typeid(Multi));
  CHECK(typeid(mp) == typeid(Multi));

  VD vd;
  V& vr = vd;
  CHECK(typeid(vr) == typeid(VD));

  // polymorphic glvalue: evaluated
  evaluations = 0;
  CHECK(typeid(counted(pr)) == typeid(Leaf));
  CHECK(evaluations == 1);

  // non-polymorphic: static type, unevaluated
  PlainDerived pd;
  Plain& plr = pd;
  evaluations = 0;
  CHECK(typeid(counted_plain(plr)) == typeid(Plain));
  CHECK(evaluations == 0);

  // prvalue of polymorphic type: static type (temporary materialization)
  CHECK(typeid(Leaf{}) == typeid(Leaf));

  // the referenced object lives to the end of the program
  const std::type_info* ti = &typeid(pr);
  CHECK(*ti == typeid(Leaf));
  CHECK(ti->name() != nullptr);
  return 0;
}
