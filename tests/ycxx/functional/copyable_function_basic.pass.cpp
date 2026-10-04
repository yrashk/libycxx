// [func.wrap.copy.class]: copyable_function is copy- and move-constructible/assignable.
// [func.wrap.copy.ctor]/2: default/nullptr: no target. /3 copy: "the target object of *this
// is a copy of the target object of f". /5 move noexcept. /10: target of type VT
// direct-non-list-initialized with std::forward<F>(f). /24-31 assignments equivalent to
// copyable_function(...).swap(*this); /28 operator=(nullptr_t) destroys the target.
// [func.wrap.copy.inv]/1 operator bool; [func.wrap.copy.util] swap, operator==(f, nullptr).
#include <functional>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Counter {
  static inline int live = 0;
  int n = 0;
  Counter() { ++live; }
  Counter(const Counter& o) : n(o.n) { ++live; }
  Counter(Counter&& o) noexcept : n(o.n) { ++live; }
  ~Counter() { --live; }
  int operator()() { return ++n; }
};
int seven() { return 7; }

using C = std::copyable_function<int()>;
static_assert(std::is_same_v<C::result_type, int>);
static_assert(std::is_nothrow_default_constructible_v<C>);
static_assert(std::is_nothrow_constructible_v<C, std::nullptr_t>);
static_assert(std::is_nothrow_move_constructible_v<C>);
static_assert(std::is_copy_constructible_v<C>);
static_assert(std::is_copy_assignable_v<C>);
static_assert(std::is_move_assignable_v<C>);
static_assert(std::is_nothrow_assignable_v<C&, std::nullptr_t>);
static_assert(noexcept(std::declval<C&>().swap(std::declval<C&>())));
static_assert(noexcept(swap(std::declval<C&>(), std::declval<C&>())));
static_assert(noexcept(static_cast<bool>(std::declval<const C&>())));
static_assert(!std::is_convertible_v<C, bool>);
static_assert(noexcept(std::declval<const C&>() == nullptr));
static_assert(std::is_same_v<decltype(std::declval<const C&>() == nullptr), bool>);

int main() {
  C empty;
  CHECK(!empty && empty == nullptr && nullptr == empty);
  C n(nullptr);
  CHECK(!n);
  C ce = empty;  // copy of an empty wrapper is empty
  CHECK(!ce);

  {
    C a = Counter{};
    CHECK(Counter::live == 1);
    CHECK(a() == 1 && a() == 2);
    C b = a;  // copies the target, including its state
    CHECK(Counter::live == 2);
    CHECK(b() == 3 && a() == 3);  // and the copies are independent
    C c = std::move(b);
    CHECK(c() == 4);
    b = a;  // copy assignment
    CHECK(b() == 4 && a() == 4);
    C& self = (b = c);
    CHECK(&self == &b && b() == 5 && c() == 5);
    b = nullptr;
    CHECK(!b);
    b = seven;
    CHECK(b() == 7);
    b = std::move(a);
    CHECK(b() == 5);
    b = Counter{};
    CHECK(b() == 1);
  }
  CHECK(Counter::live == 0);

  C x = seven, y = Counter{};
  x.swap(y);
  CHECK(x() == 1 && y() == 7);
  swap(x, y);
  CHECK(x() == 7 && y() == 2);
  return 0;
}
