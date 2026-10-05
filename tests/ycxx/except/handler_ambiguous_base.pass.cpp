// [except.handle]/3.2: a handler of type cv T or cv T& matches E if "T is an unambiguous
// public base class of E". A base that appears twice as a non-virtual base is ambiguous, so
// the handler must not match; an intermediate base that makes the path unique does match.
// REQUIRES: exceptions
#include "check.hpp"

struct Base {
  int tag;
  explicit Base(int t) : tag(t) {}
  virtual ~Base() = default;
};
struct Left : Base {
  Left() : Base(1) {}
};
struct Right : Base {
  Right() : Base(2) {}
};
struct Diamond : Left, Right {};

// Base is ambiguous in Mixed (direct non-virtual + via Left) -- compilers may warn
struct VBase {
  virtual ~VBase() = default;
};
struct VL : virtual VBase {};
struct VR : virtual VBase {};
struct NV : VBase {};
struct Mixed2 : VL, VR, NV {};  // one virtual VBase plus one non-virtual: ambiguous

int main() {
  int which = 0;
  try {
    throw Diamond();
  } catch (Base&) {
    which = -1;
  } catch (Right& r) {
    which = r.tag;
  }
  CHECK(which == 2);

  which = 0;
  try {
    throw Diamond();
  } catch (Base) {
    which = -1;
  } catch (Left l) {
    which = l.tag;
  }
  CHECK(which == 1);

  which = 0;
  try {
    throw Mixed2();
  } catch (VBase&) {
    which = -1;
  } catch (...) {
    which = 1;
  }
  CHECK(which == 1);

  which = 0;
  try {
    throw Mixed2();
  } catch (VL&) {
    which = 1;
  }
  CHECK(which == 1);
  return 0;
}
