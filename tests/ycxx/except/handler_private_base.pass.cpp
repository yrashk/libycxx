// [except.handle]/3.2: a handler of type cv T or cv T& matches E if "T is an unambiguous
// public base class of E". A private or protected base is not public, so such a handler must
// not match even where the base is accessible from the throwing context.
// REQUIRES: exceptions
#include "check.hpp"

struct Base {
  int x = 5;
  virtual ~Base() = default;
};
struct PrivDerived : private Base {
  static void raise() { throw PrivDerived(); }  // conversion accessible here: still no match
};
struct ProtDerived : protected Base {
  static void raise() { throw ProtDerived(); }
};
struct PubOfProt : ProtDerived {};
struct PubDerived : public Base {};

template <class F>
int classify(F f) {
  try {
    f();
  } catch (Base&) {
    return 1;
  } catch (...) {
    return 2;
  }
  return 0;
}

int main() {
  CHECK(classify([] { throw PubDerived(); }) == 1);
  CHECK(classify([] { PrivDerived::raise(); }) == 2);
  CHECK(classify([] { ProtDerived::raise(); }) == 2);
  CHECK(classify([] { throw PubOfProt(); }) == 2);

  // by value as well
  int which = 0;
  try {
    try {
      PrivDerived::raise();
    } catch (Base) {
      which = -1;
    }
  } catch (PrivDerived&) {
    which = 1;
  }
  CHECK(which == 1);

  // exact type still matches
  which = 0;
  try {
    ProtDerived::raise();
  } catch (const ProtDerived&) {
    which = 1;
  }
  CHECK(which == 1);
  return 0;
}
