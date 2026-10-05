// [expr.throw]/2: "The array-to-pointer ... standard conversion[] [is] performed on the
// operand." [except.throw]/1 example: "throw "Help!"; can be caught by a handler of
// const char* type". [except.handle]/2: "A handler of type 'array of T' ... is adjusted to be
// of type 'pointer to T'." [Note 1]: "A handler of reference to array ... type is never a
// match for any exception object."
// REQUIRES: exceptions
// XFAIL: gcc  GCC 16 records catch (int(&)[3]) as a handler for int*; the runtime cannot tell ([except.handle] Note 1; STATUS)
#include "check.hpp"

int main() {
  int arr[3] = {4, 5, 6};
  int which = 0;
  try {
    throw arr;
  } catch (int* p) {
    which = p == arr ? p[1] : -1;
  }
  CHECK(which == 5);

  which = 0;
  try {
    throw arr;
  } catch (int p[3]) {  // adjusted to int*
    which = p[2];
  }
  CHECK(which == 6);

  which = 0;
  try {
    throw arr;
  } catch (const int p[]) {  // adjusted to const int*
    which = p[0];
  }
  CHECK(which == 4);

  which = 0;
  try {
    try {
      throw arr;
    } catch (int (&)[3]) {
      which = -1;
    }
  } catch (...) {
    which = 1;
  }
  CHECK(which == 1);

  // string literal: exception object of type const char*
  which = 0;
  try {
    throw "Help!";
  } catch (const char* s) {
    which = s[0] == 'H' && s[4] == '!' ? 1 : -1;
  }
  CHECK(which == 1);

  which = 0;
  try {
    try {
      throw "Help!";
    } catch (char*) {
      which = -1;
    } catch (void*) {
      which = -2;
    }
  } catch (const void*) {
    which = 1;
  }
  CHECK(which == 1);

  // the decayed pointer from an array of class type behaves as a class pointer
  struct S {
    int v;
  };
  S ss[2] = {{1}, {2}};
  which = 0;
  try {
    throw ss;
  } catch (const S* p) {
    which = p[1].v;
  }
  CHECK(which == 2);
  return 0;
}
