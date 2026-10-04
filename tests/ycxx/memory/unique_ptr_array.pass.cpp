// [unique.ptr.runtime]: unique_ptr<T[], D>. /1.1-1.2: pointers to types derived from T are
// rejected by the constructors and by reset; [unique.ptr.runtime.ctor]/2,4: explicit
// unique_ptr(U p) / (U p, d) accept U == pointer, or (for pointer == element_type*) V*
// with V(*)[] convertible to element_type(*)[] — i.e. qualification conversions — and the
// (p, d) forms also nullptr_t. /6: the converting move constructor accepts unique_ptr<U[],
// E> with U(*)[] convertible to T(*)[]. [unique.ptr.runtime.modifiers]: reset(nullptr_t)
// and reset(U) with the same constraint. The default deleter calls delete[].
#include <memory>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Counted {
  static inline int live = 0;
  Counted() { ++live; }
  ~Counted() { --live; }
};
struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

struct ArrDel {
  int* calls;
  template <class T>
  void operator()(T* p) const {
    ++*calls;
    delete[] p;
  }
};

// constructor constraints
static_assert(std::is_constructible_v<std::unique_ptr<const int[]>, int*>);
static_assert(std::is_constructible_v<std::unique_ptr<const int[]>, const int*>);
static_assert(!std::is_constructible_v<std::unique_ptr<int[]>, const int*>);
static_assert(!std::is_constructible_v<std::unique_ptr<Base[]>, Derived*>);
static_assert(!std::is_constructible_v<std::unique_ptr<int[]>, long*>);
static_assert(!std::is_constructible_v<std::unique_ptr<int[]>, void*>);
static_assert(std::is_constructible_v<std::unique_ptr<int[], ArrDel>, std::nullptr_t, ArrDel>);
static_assert(std::is_constructible_v<std::unique_ptr<const int[], ArrDel>, int*, ArrDel>);
static_assert(!std::is_constructible_v<std::unique_ptr<Base[], std::default_delete<Base[]>>, Derived*,
                                       std::default_delete<Base[]>>);
// converting move construction / assignment
static_assert(std::is_constructible_v<std::unique_ptr<const int[]>, std::unique_ptr<int[]>&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<int[]>, std::unique_ptr<const int[]>&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<Base[]>, std::unique_ptr<Derived[]>&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<int[]>, std::unique_ptr<int>&&>);
static_assert(std::is_assignable_v<std::unique_ptr<const int[]>&, std::unique_ptr<int[]>&&>);
static_assert(!std::is_assignable_v<std::unique_ptr<Base[]>&, std::unique_ptr<Derived[]>&&>);
// reset constraints
template <class P, class U>
concept can_reset = requires(P p, U u) { p.reset(u); };
static_assert(can_reset<std::unique_ptr<const int[]>, int*>);
static_assert(can_reset<std::unique_ptr<int[]>, std::nullptr_t>);
static_assert(!can_reset<std::unique_ptr<Base[]>, Derived*>);
static_assert(!can_reset<std::unique_ptr<int[]>, const int*>);

int main() {
  {
    std::unique_ptr<Counted[]> p(new Counted[5]);
    CHECK(Counted::live == 5);
    p.reset(new Counted[2]);
    CHECK(Counted::live == 2);  // delete[] destroyed all five
    p = nullptr;
    CHECK(Counted::live == 0);
    p.reset(new Counted[3]);
  }
  CHECK(Counted::live == 0);
  {
    int calls = 0;
    std::unique_ptr<int[], ArrDel> a(new int[2]{1, 2}, ArrDel{&calls});
    std::unique_ptr<int[], ArrDel> b(nullptr, ArrDel{&calls});
    CHECK(!b);
    b = std::move(a);
    CHECK(calls == 0 && b[1] == 2 && !a);
    std::unique_ptr<const int[], ArrDel> c(std::move(b));
    CHECK(c[0] == 1 && !b);
    c.reset();
    CHECK(calls == 1);
  }
  {
    std::unique_ptr<const int[]> c(new int[3]{7, 8, 9});
    static_assert(std::is_same_v<decltype(c[0]), const int&>);
    CHECK(c[2] == 9);
    int* raw = new int[1]{1};
    c.reset(raw);
    CHECK(c.get() == raw);
  }
  return 0;
}
