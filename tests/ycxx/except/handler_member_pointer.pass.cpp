// [except.handle]/3.3: a pointer-to-member handler of type cv T or const T& matches an
// exception object of pointer-to-member type E convertible to T by ... "a function pointer
// conversion" ([conv.fctptr]: also pointer to noexcept member function) or "a qualification
// conversion" ([conv.qual]). (Whether the base-to-derived pointer-to-member conversion of
// [conv.mem]/2 counts as a "standard pointer conversion" is not clear, so it is not tested.)
// REQUIRES: exceptions
#include "check.hpp"

struct B {
  int x = 1;
  int y = 2;
  int g() { return 7; }
  int h() noexcept { return 8; }
};
struct Other {
  int x = 4;
};

template <class H, class E>
int catches(E e) {
  try {
    try {
      throw e;
    } catch (H) {
      return 1;
    }
  } catch (...) {
    return 0;
  }
  return -1;
}

int main() {
  B d;
  int which = 0;
  try {
    throw &B::y;
  } catch (int B::*pm) {
    which = d.*pm;
  }
  CHECK(which == 2);
  CHECK(catches<int Other::*>(&B::x) == 0);  // unrelated class
  CHECK(catches<long B::*>(&B::x) == 0);

  // qualification conversion
  which = 0;
  try {
    throw &B::x;
  } catch (const int B::*pm) {
    which = d.*pm;
  }
  CHECK(which == 1);
  CHECK(catches<const volatile int B::*>(&B::x) == 1);
  const int B::*cpm = &B::x;
  CHECK(catches<int B::*>(cpm) == 0);

  // member function pointers, with function pointer conversion (noexcept dropped)
  which = 0;
  try {
    throw &B::g;
  } catch (int (B::*const& pf)()) {
    which = (d.*pf)();
  }
  CHECK(which == 7);

  which = 0;
  try {
    throw &B::h;
  } catch (int (B::*pf)()) {
    which = (d.*pf)();
  }
  CHECK(which == 8);
  CHECK(catches<int (B::*)() noexcept>(&B::g) == 0);  // cannot add noexcept
  CHECK(catches<int (B::*)() noexcept>(&B::h) == 1);
  CHECK(catches<int (B::*)() const>(&B::g) == 0);
  CHECK(catches<int (Other::*)()>(&B::g) == 0);

  // data member pointer does not match a member function pointer handler and vice versa
  CHECK(catches<int (B::*)()>(&B::x) == 0);
  return 0;
}
