// [expr.dynamic.cast]/10: "A failed cast to reference type throws an exception of a type that
// would match a handler of type std::bad_cast." This applies to lvalue and rvalue reference
// targets ([expr.dynamic.cast]/2), and to failed downcasts, cross casts and ambiguous casts.
// [bad.cast]: bad_cast derives from exception; what() returns an NTBS.
// REQUIRES: exceptions
#include <exception>
#include <typeinfo>
#include "check.hpp"

struct Base {
  virtual ~Base() = default;
};
struct D1 : Base {};
struct D2 : Base {};
struct Side {
  virtual ~Side() = default;
};
struct L : Base {};
struct R : Base {};
struct Amb : L, R, Side {};

template <class F>
int outcome(F f) {
  try {
    f();
    return 0;
  } catch (const std::bad_cast& e) {
    return e.what() != nullptr ? 1 : -1;
  } catch (...) {
    return 2;
  }
}

int main() {
  D1 d1;
  Base& b = d1;
  CHECK(outcome([&] { (void)dynamic_cast<D2&>(b); }) == 1);
  CHECK(outcome([&] { (void)dynamic_cast<D2&&>(b); }) == 1);
  CHECK(outcome([&] { (void)dynamic_cast<const D2&>(b); }) == 1);
  CHECK(outcome([&] { (void)dynamic_cast<Side&>(b); }) == 1);
  CHECK(outcome([&] { (void)dynamic_cast<D1&>(b); }) == 0);

  // From the L::Base subobject: no R derives from it (/9.1 fails), but the most derived Amb
  // has an unambiguous public R base (/9.2): success. To Base from Side: Base is ambiguous.
  Amb a;
  L& l = a;
  CHECK(outcome([&] { (void)dynamic_cast<R&>(static_cast<Base&>(l)); }) == 0);
  Side& s = a;
  CHECK(outcome([&] { (void)dynamic_cast<Base&>(s); }) == 1);
  CHECK(outcome([&] { (void)dynamic_cast<L&>(s); }) == 0);

  // caught as std::exception too
  bool as_exception = false;
  try {
    (void)dynamic_cast<D2&>(b);
  } catch (const std::exception&) {
    as_exception = true;
  }
  CHECK(as_exception);
  return 0;
}
