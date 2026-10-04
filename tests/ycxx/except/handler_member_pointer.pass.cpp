// [except.handle]/3.3: a pointer-to-member handler of type cv T or const T& matches an
// exception object of pointer-to-member type E convertible to T by "a standard pointer
// conversion not involving conversions to pointers to private or protected or ambiguous
// classes" ([conv.mem]/2: pointer to member of B converts to pointer to member of D for a
// derived class D), "a function pointer conversion" or "a qualification conversion".
#include "check.hpp"

struct B {
  int x = 1;
  int y = 2;
  int g() { return 7; }
  int h() noexcept { return 8; }
};
struct D : B {
  int z = 3;
};
struct P : private B {};
struct L : B {};
struct R : B {};
struct Amb : L, R {};

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
  // base member pointer to derived member pointer: matches; the value designates B::y
  D d;
  int which = 0;
  try {
    throw &B::y;
  } catch (int D::*pm) {
    which = d.*pm;
  }
  CHECK(which == 2);

  CHECK(catches<int B::*>(&D::z) == 0);  // derived-to-base is not a standard conversion
  CHECK(catches<int P::*>(&B::x) == 0);  // private base
  CHECK(catches<int Amb::*>(&B::x) == 0);  // ambiguous base
  CHECK(catches<int L::*>(&B::x) == 1);
  CHECK(catches<long B::*>(&B::x) == 0);

  // qualification conversion
  which = 0;
  try {
    throw &B::x;
  } catch (const int B::*pm) {
    which = d.*pm;
  }
  CHECK(which == 1);
  CHECK(catches<const volatile int D::*>(&B::x) == 1);
  const int B::*cpm = &B::x;
  CHECK(catches<int B::*>(cpm) == 0);

  // member function pointers, with function pointer conversion (noexcept dropped)
  which = 0;
  try {
    throw &B::g;
  } catch (int (D::*pf)()) {
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
  CHECK(catches<int (D::*)() noexcept>(&B::h) == 1);
  CHECK(catches<int (B::*)() const>(&B::g) == 0);

  // data member pointer does not match a member function pointer handler and vice versa
  CHECK(catches<int (B::*)()>(&B::x) == 0);
  return 0;
}
