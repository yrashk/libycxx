// [except.handle]/3.4: "the handler is of type cv T or const T& where T is a pointer or
// pointer-to-member type and E is std::nullptr_t." The handler variable is then a null value.
// [Note 1]: "A throw-expression whose operand is an integer literal with value zero does not
// match a handler of pointer or pointer-to-member type."
#include <cstddef>
#include "check.hpp"

struct S {
  int m;
  void f() {}
  virtual ~S() = default;
};

template <class H>
int catch_null() {
  try {
    try {
      throw nullptr;
    } catch (H h) {
      return h == nullptr ? 1 : -1;
    }
  } catch (...) {
    return 0;
  }
  return -2;
}

int main() {
  CHECK(catch_null<int*>() == 1);
  CHECK(catch_null<const char*>() == 1);
  CHECK(catch_null<void*>() == 1);
  CHECK(catch_null<const volatile void*>() == 1);
  CHECK(catch_null<S*>() == 1);
  CHECK(catch_null<S* const&>() == 1);
  CHECK(catch_null<int**>() == 1);
  CHECK(catch_null<void (*)()>() == 1);
  CHECK(catch_null<void (*)() noexcept>() == 1);
  CHECK(catch_null<int S::*>() == 1);
  CHECK(catch_null<const int S::*>() == 1);
  CHECK(catch_null<void (S::*)()>() == 1);
  CHECK(catch_null<std::nullptr_t>() == 1);
  CHECK(catch_null<const std::nullptr_t&>() == 1);

  // integer zero is an int, not a null pointer
  int which = 0;
  try {
    try {
      throw 0;
    } catch (int*) {
      which = -1;
    } catch (void*) {
      which = -2;
    } catch (int S::*) {
      which = -3;
    } catch (std::nullptr_t) {
      which = -4;
    }
  } catch (int i) {
    which = i == 0 ? 1 : -5;
  }
  CHECK(which == 1);

  // a pointer exception object does not match a nullptr_t handler
  which = 0;
  try {
    try {
      throw static_cast<int*>(nullptr);
    } catch (std::nullptr_t) {
      which = -1;
    }
  } catch (int* p) {
    which = p == nullptr ? 1 : -2;
  }
  CHECK(which == 1);
  return 0;
}
