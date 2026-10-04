// [except.handle]/3 + [expr.typeid]/4: inside a handler, typeid of the caught polymorphic
// object (bound by reference) is the dynamic type of the exception object, which
// [expr.throw]/2 fixes as the static type of the throw operand. A by-value handler variable
// of base type is a sliced copy ([except.handle]/15) whose dynamic type is the base.
#include <exception>
#include <stdexcept>
#include <typeinfo>
#include "check.hpp"

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

int main() {
  try {
    throw Derived();
  } catch (Base& b) {
    CHECK(typeid(b) == typeid(Derived));
  }

  try {
    throw Derived();
  } catch (Base b) {
    CHECK(typeid(b) == typeid(Base));
  }

  // throw through a base reference: the exception object has the static type Base
  Derived d;
  Base& br = d;
  try {
    throw br;
  } catch (Base& b) {
    CHECK(typeid(b) == typeid(Base));
  }

  try {
    throw std::out_of_range("x");
  } catch (const std::exception& e) {
    CHECK(typeid(e) == typeid(std::out_of_range));
    CHECK(dynamic_cast<const std::logic_error*>(&e) != nullptr);
    CHECK(dynamic_cast<const std::runtime_error*>(&e) == nullptr);
  }
  return 0;
}
