// [expr.dynamic.cast]/6: "If v is a null pointer value, the result is a null pointer value."
// /9.1: "If, in the most derived object pointed (referred) to by v, v points (refers) to a
// public base class subobject of a C object, and if only one object of type C is derived from
// the subobject pointed (referred) to by v, the result points (refers) to that C object."
// /10: "The value of a failed cast to pointer type is the null pointer value of the required
// result type."
#include "check.hpp"

struct Base {
  int b = 1;
  virtual ~Base() = default;
};
struct Mid : Base {
  int m = 2;
};
struct Leaf : Mid {
  int l = 3;
};
struct Other : Base {};

int main() {
  Leaf leaf;
  Base* bp = &leaf;
  CHECK(dynamic_cast<Leaf*>(bp) == &leaf);
  CHECK(dynamic_cast<Mid*>(bp) == static_cast<Mid*>(&leaf));
  CHECK(dynamic_cast<Other*>(bp) == nullptr);
  CHECK(dynamic_cast<Leaf*>(static_cast<Base*>(nullptr)) == nullptr);

  Mid mid;
  Base* bm = &mid;
  CHECK(dynamic_cast<Mid*>(bm) == &mid);
  CHECK(dynamic_cast<Leaf*>(bm) == nullptr);  // object is not a Leaf

  Base base;
  CHECK(dynamic_cast<Mid*>(&base) == nullptr);

  // cv-qualifications carried over
  const Base* cbp = &leaf;
  const Leaf* clp = dynamic_cast<const Leaf*>(cbp);
  CHECK(clp == &leaf && clp->l == 3);
  const volatile Leaf* cvlp = dynamic_cast<const volatile Leaf*>(cbp);
  CHECK(cvlp == &leaf);

  // reference cast success
  Base& br = leaf;
  Leaf& lr = dynamic_cast<Leaf&>(br);
  CHECK(&lr == &leaf);
  Mid&& mr = dynamic_cast<Mid&&>(br);
  CHECK(&mr == static_cast<Mid*>(&leaf));

  // same type and upcasts (/3, /4) need no runtime check
  CHECK(dynamic_cast<Leaf*>(&leaf) == &leaf);
  CHECK(dynamic_cast<Base*>(&leaf) == static_cast<Base*>(&leaf));
  return 0;
}
