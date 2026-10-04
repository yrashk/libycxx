// [unique.ptr.single.ctor]: unique_ptr() / unique_ptr(nullptr_t) own nothing and
// value-initialize the deleter; explicit unique_ptr(p) owns p; unique_ptr(p, d) for
// D = value type (copies an lvalue d, moves an rvalue d), D = A& (refers to the lvalue d),
// D = const A&; unique_ptr(unique_ptr&&) transfers ownership and moves (or, for a reference
// D, copies the reference to) the deleter, leaving u.get() == nullptr; the converting
// constructor from unique_ptr<U, E>&& does the same.
#include <memory>
#include <utility>
#include "check.hpp"

struct Del {
  int id = 0;
  int* deletes = nullptr;
  int copied = 0;
  int moved = 0;
  constexpr Del() = default;
  constexpr Del(int i, int* d) : id(i), deletes(d) {}
  constexpr Del(const Del& o) : id(o.id), deletes(o.deletes), copied(o.copied + 1), moved(o.moved) {}
  constexpr Del(Del&& o) noexcept : id(o.id), deletes(o.deletes), copied(o.copied), moved(o.moved + 1) {}
  constexpr Del& operator=(const Del&) = default;
  constexpr Del& operator=(Del&&) = default;
  constexpr void operator()(int* p) const {
    if (deletes) ++*deletes;
    delete p;
  }
};

struct Base {
  int b = 1;
  constexpr virtual ~Base() = default;
};
struct Derived : Base {
  int d = 2;
};

constexpr bool test() {
  {
    std::unique_ptr<int> a;
    std::unique_ptr<int> b(nullptr);
    std::unique_ptr<int> c = nullptr;
    if (a.get() || b.get() || c.get() || a || b) return false;
    std::unique_ptr<int, Del> d;
    if (d.get_deleter().id != 0 || d.get_deleter().deletes) return false;
  }
  {
    int* raw = new int(5);
    std::unique_ptr<int> p(raw);
    if (p.get() != raw || *p != 5 || !p) return false;
  }
  int deletes = 0;
  {
    Del d(1, &deletes);
    std::unique_ptr<int, Del> a(new int(1), d);  // copies d
    if (a.get_deleter().id != 1 || a.get_deleter().copied != 1 || a.get_deleter().moved != 0) return false;
    std::unique_ptr<int, Del> b(new int(2), Del(2, &deletes));  // moves the temporary
    if (b.get_deleter().id != 2 || b.get_deleter().copied != 0 || b.get_deleter().moved != 1) return false;
    std::unique_ptr<int, Del&> c(new int(3), d);
    if (&c.get_deleter() != &d) return false;
    std::unique_ptr<int, const Del&> e(new int(4), d);
    if (&e.get_deleter() != &d) return false;
    std::unique_ptr<int, Del> f(nullptr, d);
    if (f.get() != nullptr) return false;
  }
  if (deletes != 4) return false;  // f owned nothing: the deleter is not called
  {
    deletes = 0;
    std::unique_ptr<int, Del> a(new int(7), Del(3, &deletes));
    int* raw = a.get();
    std::unique_ptr<int, Del> b(std::move(a));
    if (a.get() != nullptr || b.get() != raw || b.get_deleter().id != 3) return false;
    if (b.get_deleter().copied != 0) return false;  // move constructed
    Del d(4, &deletes);
    std::unique_ptr<int, Del&> r(new int(8), d);
    std::unique_ptr<int, Del&> r2(std::move(r));
    if (&r2.get_deleter() != &d || r.get() != nullptr) return false;
  }
  if (deletes != 2) return false;
  {
    Derived* raw = new Derived;
    std::unique_ptr<Derived> d(raw);
    std::unique_ptr<Base> b(std::move(d));
    if (b.get() != raw || d.get() != nullptr || b->b != 1) return false;
    std::unique_ptr<const Base> cb = std::move(b);  // implicit
    if (cb.get() != raw) return false;
  }
  return true;
}

int main() {
  CHECK(test());
  static_assert(test());
  return 0;
}
