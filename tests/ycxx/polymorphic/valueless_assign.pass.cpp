// [polymorphic.general]/1: a polymorphic object may become valueless only after it has been
// moved from. [polymorphic.assign]: move assignment with POCMA (std::allocator) takes
// ownership ("if the allocator needs updating or alloc == other.alloc is true, *this takes
// ownership of the owned object of other"), so other no longer owns an object; copy- or
// move-assigning from a valueless object makes *this valueless; copy construction from a
// valueless object gives a valueless object.
#include <memory>
#include <utility>
#include "check.hpp"

struct B {
  virtual ~B() = default;
  virtual int f() const { return 1; }
};
struct D : B {
  int f() const override { return 2; }
};

int main() {
  std::polymorphic<B> a(D{});
  std::polymorphic<B> b;
  B* pa = &*a;
  b = std::move(a);
  CHECK(&*b == pa && b->f() == 2);
  CHECK(a.valueless_after_move());
  std::polymorphic<B> c(a);  // copy of valueless
  CHECK(c.valueless_after_move());
  std::polymorphic<B> d;
  d = a;  // copy-assign from valueless
  CHECK(d.valueless_after_move());
  std::polymorphic<B> e;
  e = std::move(c);  // move-assign from valueless
  CHECK(e.valueless_after_move());
  e = b;  // a valueless object can be assigned to
  CHECK(!e.valueless_after_move() && e->f() == 2);
  e = e;
  CHECK(e->f() == 2);
  return 0;
}
