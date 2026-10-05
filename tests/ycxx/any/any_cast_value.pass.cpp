// [any.nonmembers]/4-8: T any_cast(const any&), T any_cast(any&), T any_cast(any&&).
// "Let U be the type remove_cvref_t<T>." "Returns: For the first and second overload,
// static_cast<T>(*any_cast<U>(&operand)). For the third overload,
// static_cast<T>(std::move(*any_cast<U>(&operand)))." "Throws: bad_any_cast if
// operand.type() != typeid(remove_reference_t<T>)."
// REQUIRES: exceptions
#include <any>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Moves {
  static inline int copies = 0, moves = 0;
  int v;
  Moves(int x) : v(x) {}
  Moves(const Moves& o) : v(o.v) { ++copies; }
  Moves(Moves&& o) noexcept : v(o.v) { o.v = -1; ++moves; }
};

static_assert(std::is_same_v<decltype(std::any_cast<int>(std::declval<std::any&>())), int>);
static_assert(std::is_same_v<decltype(std::any_cast<int&>(std::declval<std::any&>())), int&>);
static_assert(std::is_same_v<decltype(std::any_cast<const int&>(std::declval<const std::any&>())), const int&>);
static_assert(std::is_same_v<decltype(std::any_cast<int&&>(std::declval<std::any>())), int&&>);
// a prvalue of non-class type has its cv-qualification dropped ([expr.type]/2)
static_assert(std::is_same_v<decltype(std::any_cast<const int>(std::declval<std::any&>())), int>);

int main() {
  {
    // the example of [any.nonmembers]/8, without strings
    std::any x(5);
    CHECK(std::any_cast<int>(x) == 5);
    std::any_cast<int&>(x) = 10;
    CHECK(std::any_cast<int>(x) == 10);
    x = "Meow";
    CHECK(std::any_cast<const char*>(x)[0] == 'M');
    std::any_cast<const char*&>(x) = "Harry";
    CHECK(std::any_cast<const char*>(x)[0] == 'H');
  }
  {
    const std::any c(7);
    CHECK(std::any_cast<int>(c) == 7);
    CHECK(std::any_cast<const int&>(c) == 7);
    CHECK(&std::any_cast<const int&>(c) == std::any_cast<int>(&c));
  }
  {
    std::any x(1);
    // cv-qualified T: typeid ignores top-level cv
    CHECK(std::any_cast<const int>(x) == 1);
    CHECK(std::any_cast<const volatile int&>(x) == 1);
  }
  {
    std::any x = Moves(3);
    Moves::copies = Moves::moves = 0;
    Moves m = std::any_cast<Moves>(x);  // lvalue operand: copy
    CHECK(m.v == 3);
    CHECK(Moves::copies == 1);
    CHECK(Moves::moves == 0);
    Moves m2 = std::any_cast<Moves>(std::move(x));  // rvalue operand: move out
    CHECK(m2.v == 3);
    CHECK(Moves::moves == 1);
    CHECK(Moves::copies == 1);
    CHECK(x.has_value());
    CHECK(std::any_cast<Moves&>(x).v == -1);
  }
  {
    std::any x = Moves(4);
    Moves&& r = std::any_cast<Moves&&>(std::move(x));
    CHECK(&r == std::any_cast<Moves>(&x));
    const Moves& cr = std::any_cast<const Moves&>(std::move(x));
    CHECK(&cr == &r);
  }
  {
    std::any x(1);
    int n = 0;
    try {
      (void)std::any_cast<long>(x);
    } catch (const std::bad_any_cast&) {
      ++n;
    }
    try {
      (void)std::any_cast<long&>(x);
    } catch (const std::bad_cast&) {
      ++n;
    }
    try {
      (void)std::any_cast<unsigned>(std::as_const(x));
    } catch (const std::bad_any_cast&) {
      ++n;
    }
    try {
      (void)std::any_cast<int*>(std::move(x));
    } catch (const std::bad_any_cast&) {
      ++n;
    }
    std::any empty;
    try {
      (void)std::any_cast<int>(empty);
    } catch (const std::bad_any_cast&) {
      ++n;
    }
    CHECK(n == 5);
  }
  return 0;
}
