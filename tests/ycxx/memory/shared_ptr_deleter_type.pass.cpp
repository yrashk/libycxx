// [util.smartptr.shared.const]/6: template<class Y> explicit shared_ptr(Y* p) "constructs a
// shared_ptr object that owns the pointer p", and the destructor ([util.smartptr.shared.dest]
// /1.3) "Otherwise, *this owns a pointer p, and delete p is called." p has type Y*, so the
// object is destroyed as a Y even when T is void or a base without a virtual destructor, and
// even after conversion, aliasing, reset or assignment of the owners.
#include <memory>
#include "check.hpp"

struct NonVirtualBase {
  int b = 1;
  ~NonVirtualBase() { ++base_dtors; }
  static inline int base_dtors = 0;
};
struct Derived : NonVirtualBase {
  ~Derived() { ++derived_dtors; }
  static inline int derived_dtors = 0;
};
struct Thing {
  static inline int dtors = 0;
  ~Thing() { ++dtors; }
};

int main() {
  {
    std::shared_ptr<NonVirtualBase> p(new Derived);
    CHECK(p->b == 1);
  }
  CHECK(Derived::derived_dtors == 1 && NonVirtualBase::base_dtors == 1);

  {
    std::shared_ptr<void> v(new Thing);
  }
  CHECK(Thing::dtors == 1);

  // ownership passes through conversions and aliases; the last owner deletes the Thing
  {
    std::shared_ptr<void> v;
    {
      std::shared_ptr<Thing> t(new Thing);
      v = t;
      std::shared_ptr<const void> cv(t, static_cast<const void*>(nullptr));
      CHECK(cv.use_count() == 3);
    }
    CHECK(Thing::dtors == 1);
  }
  CHECK(Thing::dtors == 2);

  {
    std::shared_ptr<NonVirtualBase> p;
    p.reset(new Derived);
    std::shared_ptr<void> v = std::move(p);
    CHECK(!p && v.use_count() == 1);
  }
  CHECK(Derived::derived_dtors == 2);

  // make_shared likewise destroys the object as the type it constructed
  {
    std::shared_ptr<void> v = std::make_shared<Thing>();
  }
  CHECK(Thing::dtors == 3);
  {
    std::shared_ptr<NonVirtualBase> b = std::make_shared<Derived>();
  }
  CHECK(Derived::derived_dtors == 3);
  return 0;
}
