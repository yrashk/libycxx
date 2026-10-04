// [except.handle]/3.1: "The handler is of type cv T or cv T& and E and T are the same type
// (ignoring the top-level cv-qualifiers)". [expr.throw]/2: "The type of the exception object
// is determined by removing any top-level cv-qualifiers from the type of the (possibly
// converted) operand." So a const operand yields a non-const exception object, which a
// handler of type T& can bind to, and cv-qualified handlers match unqualified objects.
#include "check.hpp"

struct S {
  int v;
};
enum E { e1, e2 };

int main() {
  int which = 0;
  // const operand: exception object is int, caught by int&
  try {
    const int ci = 7;
    throw ci;
  } catch (int& r) {
    which = r;
    r = 8;  // the exception object is not const
  }
  CHECK(which == 7);

  which = 0;
  try {
    throw S{3};
  } catch (const volatile S& s) {
    which = s.v;
  }
  CHECK(which == 3);

  which = 0;
  try {
    throw S{4};
  } catch (const S s) {
    which = s.v;
  }
  CHECK(which == 4);

  // integral types do not convert: long handler does not match int
  which = 0;
  try {
    try {
      throw 5;
    } catch (long) {
      which = -1;
    } catch (unsigned) {
      which = -2;
    } catch (short) {
      which = -3;
    }
  } catch (int i) {
    which = i;
  }
  CHECK(which == 5);

  // enum does not match its underlying type, nor int match the enum
  which = 0;
  try {
    try {
      throw e2;
    } catch (int) {
      which = -1;
    }
  } catch (E e) {
    which = e == e2 ? 1 : -2;
  }
  CHECK(which == 1);

  // floating types do not convert either
  which = 0;
  try {
    try {
      throw 1.5f;
    } catch (double) {
      which = -1;
    }
  } catch (float f) {
    which = f == 1.5f ? 1 : -2;
  }
  CHECK(which == 1);

  // bool, char are distinct
  which = 0;
  try {
    try {
      throw 'x';
    } catch (signed char) {
      which = -1;
    } catch (unsigned char) {
      which = -2;
    } catch (int) {
      which = -3;
    }
  } catch (char c) {
    which = c == 'x' ? 1 : -4;
  }
  CHECK(which == 1);
  return 0;
}
