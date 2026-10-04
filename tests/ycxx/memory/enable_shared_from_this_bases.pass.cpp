// [util.smartptr.shared.const]/1: "enables shared_from_this with p, for a pointer p of type
// Y*, means that if Y has an unambiguous and accessible base class that is a specialization of
// enable_shared_from_this, then ... the constructor evaluates the statement:
//   if (p != nullptr && p->weak-this.expired())
//     p->weak-this = shared_ptr<remove_cv_t<Y>>(*this, const_cast<remove_cv_t<Y>*>(p));"
// Consequences checked here:
// - the base may be a second, non-primary base (Y = B : Other, A) and the shared_ptr's T may
//   be another base (Other) or void or const B: Y decides, not T;
// - an enable_shared_from_this base reached through virtual inheritance from two paths is
//   unambiguous: enabled;
// - two copies of the *same* specialization (non-virtual diamond) are ambiguous: nothing is
//   enabled, weak_from_this() stays empty and shared_from_this() throws bad_weak_ptr
//   ([util.smartptr.enab]/3 shared_ptr<T>(weak-this), [util.smartptr.shared.const]/29);
// - a private base is not accessible: not enabled;
// - "p->weak-this.expired()": while a first owner is alive, a second, unrelated owner does
//   not take over weak-this; once the first owner has expired, the next owner does.
// make_shared ([util.smartptr.shared.create]) and the unique_ptr conversion enable it too.
#include <memory>
#include "check.hpp"

struct A : std::enable_shared_from_this<A> {
  int a = 1;
  virtual ~A() = default;
};
struct Other {
  int o = 2;
  virtual ~Other() = default;
};
struct B : Other, A {
  int b = 3;
};

struct Left : A {};
struct Right : A {};
struct Diamond : Left, Right {};  // two A subobjects: enable_shared_from_this<A> is ambiguous

struct VA : std::enable_shared_from_this<VA> {
  virtual ~VA() = default;
};
struct VB : virtual VA {};
struct VC : virtual VA {};
struct VD : VB, VC {
  int d = 4;
};

struct Priv : private std::enable_shared_from_this<Priv> {
  bool enabled() { return !weak_from_this().expired(); }
};

static bool throws_bad_weak_ptr(A& a) {
  try {
    (void)a.shared_from_this();
  } catch (const std::bad_weak_ptr&) {
    return true;
  }
  return false;
}

int main() {
  {
    auto b = std::make_shared<B>();
    std::shared_ptr<A> sa = b->shared_from_this();
    CHECK(sa.get() == static_cast<A*>(b.get()) && b.use_count() == 2 && sa.owner_equal(b));
    std::shared_ptr<B> b2(new B);
    CHECK(b2->shared_from_this().owner_equal(b2));
    std::shared_ptr<Other> o(new B);  // T = Other, Y = B
    CHECK(static_cast<B*>(o.get())->shared_from_this().owner_equal(o));
    std::shared_ptr<void> v(new B);  // T = void
    CHECK(static_cast<B*>(v.get())->shared_from_this().owner_equal(v));
    std::shared_ptr<const B> cb(new B);  // remove_cv_t<Y>
    CHECK(cb->shared_from_this().owner_equal(cb));
    std::unique_ptr<B> ub(new B);
    B* raw = ub.get();
    std::shared_ptr<Other> fu(std::move(ub));
    CHECK(fu.use_count() == 1 && raw->shared_from_this().owner_equal(fu));
  }
  {
    std::shared_ptr<Diamond> d(new Diamond);
    A& left = static_cast<Left&>(*d);
    A& right = static_cast<Right&>(*d);
    CHECK(left.weak_from_this().expired() && right.weak_from_this().expired());
    CHECK(throws_bad_weak_ptr(left) && throws_bad_weak_ptr(right));
    auto m = std::make_shared<Diamond>();
    CHECK(static_cast<Left&>(*m).weak_from_this().expired());
  }
  {
    std::shared_ptr<VD> d(new VD);
    CHECK(d->shared_from_this().get() == static_cast<VA*>(d.get()) && d->shared_from_this().owner_equal(d));
    auto d2 = std::make_shared<VD>();
    CHECK(d2->shared_from_this().owner_equal(d2));
    std::shared_ptr<VB> d3(new VD);
    CHECK(!d3->weak_from_this().expired());
  }
  {
    std::shared_ptr<Priv> p(new Priv);
    CHECK(!p->enabled());
    auto p2 = std::make_shared<Priv>();
    CHECK(!p2->enabled());
  }
  {
    A a;  // owned only through no-op deleters
    auto noop = [](A*) {};
    {
      std::shared_ptr<A> first(&a, noop);
      std::shared_ptr<A> second(&a, noop);  // weak-this not expired: unchanged
      CHECK(a.shared_from_this().owner_equal(first) && !a.shared_from_this().owner_equal(second));
    }
    CHECK(a.weak_from_this().expired());
    std::shared_ptr<A> third(&a, noop);  // expired: reassigned
    CHECK(a.shared_from_this().owner_equal(third));
    std::shared_ptr<A> alias(third, &a);  // aliasing constructor: does not enable anything
    CHECK(a.shared_from_this().owner_equal(third));
  }
  return 0;
}
