// [expr.dynamic.cast]/9: dynamic_cast performs downcasts and cross casts from virtual base
// class subobjects (which static_cast cannot do, [expr.static.cast]/11). The draft's Example 2
// h(): F f; A* ap = &f; "E* ep1 = dynamic_cast<E*>(ap); // succeeds" through a virtual base.
#include "check.hpp"

struct V {
  int v = 1;
  virtual ~V() = default;
};
struct L : virtual V {
  int l = 2;
};
struct R : virtual V {
  int r = 3;
};
struct D : L, R {
  int d = 4;
};
struct DD : D {
  int dd = 5;
};

int main() {
  D d;
  V* vp = &d;  // the single shared V
  CHECK(dynamic_cast<D*>(vp) == &d);
  CHECK(dynamic_cast<L*>(vp) == static_cast<L*>(&d));
  CHECK(dynamic_cast<R*>(vp) == static_cast<R*>(&d));
  CHECK(dynamic_cast<DD*>(vp) == nullptr);
  CHECK(dynamic_cast<R*>(static_cast<L*>(&d)) == static_cast<R*>(&d));

  DD dd;
  V& vr = dd;
  CHECK(&dynamic_cast<DD&>(vr) == &dd);
  CHECK(dynamic_cast<D&>(vr).d == 4);
  CHECK(dynamic_cast<L*>(&vr)->l == 2);

  // an object whose most derived type is L alone
  L lonly;
  V* lv = &lonly;
  CHECK(dynamic_cast<L*>(lv) == &lonly);
  CHECK(dynamic_cast<R*>(lv) == nullptr);
  CHECK(dynamic_cast<D*>(lv) == nullptr);
  return 0;
}
