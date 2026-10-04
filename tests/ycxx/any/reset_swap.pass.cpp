// [any.modifiers]/17-19: reset() noexcept destroys the contained value, then has_value() is
// false; swap(any&) noexcept "Exchanges the states of *this and rhs."
// [any.nonmembers]/1: swap(x, y) is equivalent to x.swap(y).
#include <any>
#include <typeinfo>
#include <utility>
#include "check.hpp"

struct Big {
  static inline int live = 0;
  char buf[256];
  int v;
  Big(int x) : buf{}, v(x) { ++live; }
  Big(const Big& o) : buf{}, v(o.v) { ++live; }
  Big(Big&& o) noexcept : buf{}, v(o.v) { ++live; }
  ~Big() { --live; }
};

int main() {
  {
    std::any a = Big(1);
    CHECK(Big::live == 1);
    a.reset();
    CHECK(!a.has_value());
    CHECK(a.type() == typeid(void));
    CHECK(Big::live == 0);
    a.reset();
    CHECK(!a.has_value());
  }
  {
    std::any a = 1, b = Big(2);
    a.swap(b);
    CHECK(a.type() == typeid(Big));
    CHECK(std::any_cast<Big&>(a).v == 2);
    CHECK(std::any_cast<int>(b) == 1);
    swap(a, b);
    CHECK(std::any_cast<int>(a) == 1);
    CHECK(std::any_cast<Big&>(b).v == 2);
    std::swap(a, b);
    CHECK(std::any_cast<Big&>(a).v == 2);
  }
  {
    std::any a = 1, empty;
    a.swap(empty);
    CHECK(!a.has_value());
    CHECK(std::any_cast<int>(empty) == 1);
    std::any e1, e2;
    e1.swap(e2);
    CHECK(!e1.has_value() && !e2.has_value());
  }
  {
    std::any a = Big(3), b = Big(4);
    a.swap(b);
    CHECK(std::any_cast<Big&>(a).v == 4);
    CHECK(std::any_cast<Big&>(b).v == 3);
    a.swap(a);
    CHECK(std::any_cast<Big&>(a).v == 4);
  }
  CHECK(Big::live == 0);
  return 0;
}
