// [except.nested]/6: "Let U be decay_t<T>." /8: "Throws: If is_class_v<U> && !is_final_v<U> &&
// !is_base_of_v<nested_exception, U> is true, an exception of unspecified type that is publicly
// derived from both U and nested_exception and constructed from std::forward<T>(t), otherwise
// std::forward<T>(t)." So: arrays and functions decay (a handler for the decayed pointer type
// catches them), unions and enumerations are not classes (thrown as-is), and a derived object
// passed through a base-class reference is sliced to U.
// REQUIRES: exceptions
#include <exception>
#include "check.hpp"

union Un {
  int i;
};
enum class En { a, b };
struct Base {
  int v = 1;
  virtual ~Base() = default;
};
struct Derived : Base {
  Derived() { v = 2; }
};
int func() { return 7; }

int main() {
  int stage = 0;
  try {
    std::throw_with_nested("lit");  // T = const char(&)[4], U = const char*
  } catch (const std::nested_exception&) {
    CHECK(false);
  } catch (const char* s) {
    CHECK(s[0] == 'l');
    ++stage;
  }
  try {
    std::throw_with_nested(func);  // U = int(*)()
  } catch (int (*f)()) {
    CHECK(f() == 7);
    ++stage;
  }
  try {
    std::throw_with_nested(Un{3});
  } catch (const std::nested_exception&) {
    CHECK(false);
  } catch (const Un& u) {
    CHECK(u.i == 3);
    ++stage;
  }
  try {
    std::throw_with_nested(En::b);
  } catch (const std::nested_exception&) {
    CHECK(false);
  } catch (En e) {
    CHECK(e == En::b);
    ++stage;
  }
  try {
    try {
      throw 0;
    } catch (...) {
      Derived d;
      const Base& b = d;
      std::throw_with_nested(b);  // U = Base: the thrown object is derived from Base, not Derived
    }
  } catch (const Derived&) {
    CHECK(false);
  } catch (const Base& b) {
    CHECK(b.v == 2);  // copy of the Base subobject
    CHECK(dynamic_cast<const std::nested_exception*>(&b) != nullptr);
    ++stage;
  }
  // const lvalue: U drops the cv-qualification, the thrown object still derives from U
  try {
    const Base cb;
    std::throw_with_nested(cb);
  } catch (Base& b) {
    CHECK(b.v == 1);
    ++stage;
  }
  CHECK(stage == 6);
  return 0;
}
