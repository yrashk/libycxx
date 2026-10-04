// [unique.ptr.dltr.dflt]: default_delete<T> is default constructible (noexcept), converts
// from default_delete<U> when U* is implicitly convertible to T*, and operator()(T*)
// calls delete. [unique.ptr.dltr.dflt1]: default_delete<T[]> converts from
// default_delete<U[]> when U(*)[] is convertible to T(*)[], and its operator() template
// accepts U* under the same condition and calls delete[]. All constexpr.
#include <memory>
#include <type_traits>
#include "check.hpp"

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {
  static inline int dtors = 0;
  ~Derived() override { ++dtors; }
};
struct Counted {
  static inline int dtors = 0;
  ~Counted() { ++dtors; }
};

static_assert(std::is_nothrow_default_constructible_v<std::default_delete<int>>);
static_assert(std::is_nothrow_constructible_v<std::default_delete<Base>, const std::default_delete<Derived>&>);
static_assert(std::is_convertible_v<std::default_delete<Derived>, std::default_delete<Base>>);
static_assert(!std::is_constructible_v<std::default_delete<Derived>, std::default_delete<Base>>);
static_assert(std::is_convertible_v<std::default_delete<int>, std::default_delete<const int>>);
static_assert(!std::is_constructible_v<std::default_delete<int>, std::default_delete<const int>>);
static_assert(std::is_convertible_v<std::default_delete<int[]>, std::default_delete<const int[]>>);
static_assert(!std::is_constructible_v<std::default_delete<Base[]>, std::default_delete<Derived[]>>);
static_assert(!std::is_constructible_v<std::default_delete<int[]>, std::default_delete<int>>);
static_assert(std::is_invocable_v<std::default_delete<const int[]>, int*>);
static_assert(std::is_invocable_v<std::default_delete<const int[]>, const int*>);
static_assert(!std::is_invocable_v<std::default_delete<int[]>, const int*>);
static_assert(!std::is_invocable_v<std::default_delete<Base[]>, Derived*>);
static_assert(std::is_invocable_v<std::default_delete<Base>, Derived*>);

constexpr bool test() {
  std::default_delete<int> d;
  d(new int(1));
  std::default_delete<int[]> da;
  da(new int[3]);
  std::default_delete<const int[]> dc(da);
  dc(new int[2]);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::default_delete<Base> db = std::default_delete<Derived>();
  db(new Derived);  // virtual destructor: Derived's destructor runs
  CHECK(Derived::dtors == 1);
  std::default_delete<Counted[]>()(new Counted[4]);
  CHECK(Counted::dtors == 4);
  return 0;
}
