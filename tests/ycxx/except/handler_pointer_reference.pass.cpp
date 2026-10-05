// [except.handle]/3.1 vs /3.3: a handler of type cv T& matches only when E and T are the
// same type (/3.1). The pointer conversions of /3.3 apply only to handlers of type "cv T or
// const T&". So catch (Base*&) does not match a thrown Derived*, catch (const int*&) and
// catch (void*&) do not match a thrown int*, while catch (Base* const&) does.
// REQUIRES: exceptions
#include "check.hpp"

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

int main() {
  Derived d;
  Derived* pd = &d;
  int which = 0;
  try {
    try {
      throw pd;
    } catch (Base*&) {
      which = -1;
    }
  } catch (Base* const& p) {
    which = p == pd ? 1 : -2;
  }
  CHECK(which == 1);

  int i = 0;
  which = 0;
  try {
    try {
      throw &i;
    } catch (const int*&) {
      which = -1;
    } catch (void*&) {
      which = -2;
    }
  } catch (const int* const& p) {
    which = p == &i ? 1 : -3;
  }
  CHECK(which == 1);
  return 0;
}
