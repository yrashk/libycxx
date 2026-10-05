// [unique.ptr.single.observers]: operator* returns *get(), operator-> returns get(), get()
// returns the stored pointer, get_deleter() a reference to the stored deleter (const
// overload for const unique_ptr), explicit operator bool is get() != nullptr.
// [unique.ptr.runtime.observers]: operator[](i) returns get()[i] as T&.
// COUNTERPART: libcxx:utilities/smartptr/unique.ptr/unique.ptr.class/unique.ptr.observers/dereference.single.pass.cpp
#include <memory>
#include "check.hpp"

struct Point {
  int x = 1;
  int y = 2;
  constexpr int sum() const { return x + y; }
};

struct Del {
  int state = 0;
  constexpr void operator()(Point* p) const { delete p; }
};

constexpr bool test() {
  std::unique_ptr<Point, Del> p(new Point);
  if (!p || p->sum() != 3) return false;
  (*p).x = 10;
  if (p.get()->x != 10 || p->sum() != 12) return false;
  if (&*p != p.get() || p.operator->() != p.get()) return false;
  p.get_deleter().state = 4;
  const auto& cp = p;
  if (cp.get_deleter().state != 4 || &cp.get_deleter() != &p.get_deleter()) return false;
  cp->x = 11;  // constness of the unique_ptr does not propagate to the pointee
  if (p->x != 11) return false;
  bool b = static_cast<bool>(p);
  if (!b) return false;
  p.reset();
  if (p) return false;
  if (!p == false) return false;

  const std::unique_ptr<int[]> a(new int[4]{1, 2, 3, 4});
  a[2] = 30;  // operator[] is const and returns T&
  if (a[0] + a[1] + a[2] + a[3] != 37) return false;
  if (&a[3] != a.get() + 3) return false;
  return true;
}

int main() {
  CHECK(test());
  static_assert(test());
  return 0;
}
