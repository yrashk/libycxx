// [any.assign]: operator=(const any&) "As if by any(rhs).swap(*this)"; operator=(any&&)
// noexcept, "Postconditions: The state of *this is equivalent to the original state of rhs";
// operator=(T&&) "Constructs an object tmp of type any that contains an object of type VT
// direct-initialized with std::forward<T>(rhs), and tmp.swap(*this)". All return *this.
#include <any>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Counted {
  static inline int live = 0;
  int v;
  Counted(int x) : v(x) { ++live; }
  Counted(const Counted& o) : v(o.v) { ++live; }
  ~Counted() { --live; }
};

static_assert(std::is_same_v<decltype(std::declval<std::any&>() = std::declval<const std::any&>()), std::any&>);
static_assert(std::is_same_v<decltype(std::declval<std::any&>() = std::declval<std::any>()), std::any&>);
static_assert(std::is_same_v<decltype(std::declval<std::any&>() = 1), std::any&>);
static_assert(noexcept(std::declval<std::any&>() = std::declval<std::any>()));

int main() {
  {
    std::any a = 1, b = 2.5;
    std::any& r = (a = b);
    CHECK(&r == &a);
    CHECK(a.type() == typeid(double));
    CHECK(std::any_cast<double>(a) == 2.5);
    CHECK(std::any_cast<double>(b) == 2.5);
    CHECK(std::any_cast<double>(&a) != std::any_cast<double>(&b));
  }
  {
    std::any a = 1, empty;
    a = empty;
    CHECK(!a.has_value());
    a = std::any(5);
    CHECK(std::any_cast<int>(a) == 5);
  }
  {
    std::any a, b = Counted(3);
    std::any& r = (a = std::move(b));
    CHECK(&r == &a);
    CHECK(a.type() == typeid(Counted));
    CHECK(std::any_cast<Counted&>(a).v == 3);
  }
  {
    std::any a = 1, empty;
    a = std::move(empty);
    CHECK(!a.has_value());
  }
  {
    std::any a = Counted(1);
    std::any& r = (a = 7L);
    CHECK(&r == &a);
    CHECK(a.type() == typeid(long));
    CHECK(Counted::live == 0);  // old value destroyed
    a = "x";
    CHECK(a.type() == typeid(const char*));
    int arr[2] = {};
    a = arr;
    CHECK(a.type() == typeid(int*));
  }
  {
    std::any a = Counted(4);
    a = a;  // self copy-assignment keeps the value
    CHECK(a.has_value());
    CHECK(std::any_cast<Counted&>(a).v == 4);
  }
  CHECK(Counted::live == 0);
  return 0;
}
