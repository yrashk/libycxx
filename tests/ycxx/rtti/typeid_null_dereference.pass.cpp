// [expr.typeid]/3: "If an expression operand of typeid is a possibly-parenthesized
// unary-expression whose unary-operator is * and whose operand evaluates to a null pointer
// value, the typeid expression throws an exception of a type that would match a handler of
// type std::bad_typeid." [bad.typeid]: bad_typeid derives from exception.
#include <exception>
#include <typeinfo>
#include "check.hpp"

struct Poly {
  virtual ~Poly() = default;
};
struct Derived : Poly {};
struct Side {
  virtual ~Side() = default;
};
struct Both : Side, Derived {};

template <class F>
int outcome(F f) {
  try {
    f();
    return 0;
  } catch (const std::bad_typeid& e) {
    return e.what() != nullptr ? 1 : -1;
  } catch (...) {
    return 2;
  }
}

int main() {
  Poly* null_poly = nullptr;
  const Derived* null_derived = nullptr;
  Side* null_side = nullptr;
  CHECK(outcome([&] { (void)typeid(*null_poly); }) == 1);
  CHECK(outcome([&] { (void)typeid((*null_poly)); }) == 1);
  CHECK(outcome([&] { (void)typeid(((*null_derived))); }) == 1);
  CHECK(outcome([&] { (void)typeid(*null_side); }) == 1);

  // a pointer that becomes null after conversion from a null derived pointer
  Both* nb = nullptr;
  CHECK(outcome([&] { (void)typeid(*static_cast<Side*>(nb)); }) == 1);

  // non-null: no exception
  Both b;
  Side* sp = &b;
  CHECK(outcome([&] { CHECK(typeid(*sp) == typeid(Both)); }) == 0);

  // caught by a handler for std::exception
  bool as_exception = false;
  try {
    (void)typeid(*null_poly);
  } catch (const std::exception&) {
    as_exception = true;
  }
  CHECK(as_exception);
  return 0;
}
