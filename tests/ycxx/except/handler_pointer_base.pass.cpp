// [except.handle]/3.3: a handler of type cv T or const T&, T a pointer type, matches an
// exception object of pointer type E "that can be converted to T by one or more of a standard
// pointer conversion not involving conversions to pointers to private or protected or
// ambiguous classes, ... a qualification conversion". /15.2: the handler variable is
// copy-initialized from the exception object, i.e. the pointer is converted (adjusted).
#include "check.hpp"

struct Base {
  int b = 1;
  virtual ~Base() = default;
};
struct Other {
  int o = 2;
  virtual ~Other() = default;
};
struct Derived : Other, Base {
  int d = 3;
};
struct Priv : private Base {
  static void raise(Priv* p) { throw p; }
};
struct L : Base {};
struct R : Base {};
struct Amb : L, R {};

int main() {
  Derived d;
  int which = 0;
  try {
    throw &d;
  } catch (Base* p) {
    CHECK(p == static_cast<Base*>(&d));  // adjusted to the Base subobject
    which = p->b;
  }
  CHECK(which == 1);

  which = 0;
  try {
    throw &d;
  } catch (const Base* const p) {  // pointer conversion + qualification conversion
    CHECK(p == static_cast<const Base*>(&d));
    which = 1;
  }
  CHECK(which == 1);

  which = 0;
  try {
    throw &d;
  } catch (Base* const& p) {  // const T&
    CHECK(p == static_cast<Base*>(&d));
    which = 1;
  }
  CHECK(which == 1);

  which = 0;
  try {
    throw &d;
  } catch (Other* p) {
    CHECK(p == static_cast<Other*>(&d));
    which = 1;
  }
  CHECK(which == 1);

  // null derived pointer converts to null base pointer
  which = 0;
  try {
    throw static_cast<Derived*>(nullptr);
  } catch (Base* p) {
    which = p == nullptr ? 1 : -1;
  }
  CHECK(which == 1);

  // base pointer does not convert to derived pointer
  which = 0;
  try {
    try {
      throw static_cast<Base*>(&d);
    } catch (Derived*) {
      which = -1;
    }
  } catch (Base*) {
    which = 1;
  }
  CHECK(which == 1);

  // private base: not a match
  Priv pv;
  which = 0;
  try {
    try {
      Priv::raise(&pv);
    } catch (Base*) {
      which = -1;
    }
  } catch (Priv*) {
    which = 1;
  }
  CHECK(which == 1);

  // ambiguous base: not a match
  Amb a;
  which = 0;
  try {
    try {
      throw &a;
    } catch (Base*) {
      which = -1;
    }
  } catch (L* l) {
    CHECK(l == static_cast<L*>(&a));
    which = 1;
  }
  CHECK(which == 1);

  // const-qualified pointee cannot lose its qualification
  const Derived cd;
  which = 0;
  try {
    try {
      throw &cd;
    } catch (Base*) {
      which = -1;
    } catch (Derived*) {
      which = -2;
    }
  } catch (const Base* p) {
    CHECK(p == static_cast<const Base*>(&cd));
    which = 1;
  }
  CHECK(which == 1);
  return 0;
}
