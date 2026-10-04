// [ptr.launder]: template<class T> constexpr T* launder(T* p) noexcept; "Returns: A value of
// type T* that points to X", where X is the object within its lifetime located at the address
// p represents. Example 1 of [ptr.launder]: after replacing an object with a const member by
// placement new, the old pointer cannot be used to reach the new object, but
// std::launder(p) can.
#include <new>
#include <type_traits>
#include "check.hpp"

struct X {
  const int n;
};

struct Base {
  virtual int f() { return 1; }
  virtual ~Base() = default;
};
struct Other : Base {
  int f() override { return 2; }
};

constexpr int in_constexpr() {
  int i = 3;
  int* q = std::launder(&i);
  return *q;
}
static_assert(in_constexpr() == 3);
static_assert(std::is_same_v<decltype(std::launder(static_cast<X*>(nullptr))), X*>);
static_assert(std::is_same_v<decltype(std::launder(static_cast<const volatile int*>(nullptr))), const volatile int*>);

int main() {
  alignas(X) unsigned char buf[sizeof(X)];
  X* p = new (buf) X{3};
  CHECK(p->n == 3);
  new (p) X{5};  // a new object with a const member at the same address
  X* lp = std::launder(p);
  CHECK(lp == p);  // same address
  CHECK(lp->n == 5);
  CHECK(std::launder(reinterpret_cast<X*>(buf))->n == 5);

  alignas(Other) unsigned char poly[sizeof(Other)];
  Base* b = new (poly) Base;
  CHECK(b->f() == 1);
  b->~Base();
  new (poly) Other;  // a different dynamic type at the same address
  Base* lb = std::launder(reinterpret_cast<Base*>(poly));
  CHECK(lb->f() == 2);
  lb->~Base();
  return 0;
}
