// [except.nested]/9: rethrow_if_nested(const E& e): "If E is not a polymorphic class type, or
// if nested_exception is an inaccessible or ambiguous base class of E, there is no effect.
// Otherwise, performs: if (auto p = dynamic_cast<const nested_exception*>(addressof(e)))
// p->rethrow_nested();" So non-class E (int, pointers, enums, unions) is accepted with no
// effect; a virtual (shared, hence unambiguous) nested_exception base is used; and when E does
// not itself name nested_exception as a base, the dynamic_cast is a cross-cast on the dynamic
// type, which yields null (no effect) if that base is ambiguous or not public there.
// REQUIRES: exceptions
#include <exception>
#include "check.hpp"

enum E1 { e1 };
union U1 {
  int i;
};
struct Plain {
  int x = 0;
};
struct ProtectedNested : protected std::nested_exception {};
struct VA : virtual std::nested_exception {};
struct VB : virtual std::nested_exception {};
struct Diamond : VA, VB {};
struct Base {
  virtual ~Base() = default;
};
struct NA : std::nested_exception {};
struct NB : std::nested_exception {};
struct DynAmbiguous : Base, NA, NB {};
struct DynPrivate : Base, private std::nested_exception {};
struct DynPublic : Base, std::nested_exception {};

template <class T>
bool rethrows(const T& e) {
  try {
    std::rethrow_if_nested(e);
  } catch (int i) {
    return i == 17;
  }
  return false;
}

int main() {
  try {
    throw 17;
  } catch (...) {
    int i = 3;
    int* ptr = &i;
    CHECK(!rethrows(i));
    CHECK(!rethrows(ptr));
    CHECK(!rethrows(e1));
    CHECK(!rethrows(U1{1}));
    CHECK(!rethrows(Plain{}));
    CHECK(!rethrows(ProtectedNested{}));

    Diamond d;  // one shared nested_exception subobject, captured 17
    CHECK(rethrows(d));

    std::nested_exception ne;
    CHECK(rethrows(ne));

    DynAmbiguous da;
    CHECK(!rethrows(static_cast<const Base&>(da)));
    DynPrivate dp;
    CHECK(!rethrows(static_cast<const Base&>(dp)));
    DynPublic dpub;
    CHECK(rethrows(static_cast<const Base&>(dpub)));
  }
  return 0;
}
