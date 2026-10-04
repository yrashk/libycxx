// [func.wrap.move.class]: result_type = R; default, nullptr_t and move constructors are
// noexcept. [func.wrap.move.ctor]/2: "Postconditions: *this has no target object." /3 move:
// "The target object of *this is the target object f had before construction". /8: "*this has
// a target object of type VT direct-non-list-initialized with std::forward<F>(f)". /22,26
// assignment "Equivalent to: move_only_function(...).swap(*this)"; /24 operator=(nullptr_t)
// "Destroys the target object of *this, if any."; /28 destructor destroys the target.
// [func.wrap.move.inv]/1 operator bool; [func.wrap.move.util] swap, operator==(f, nullptr).
#include <functional>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Tracked {
  static inline int live = 0;
  int v;
  explicit Tracked(int x) : v(x) { ++live; }
  Tracked(Tracked&& o) noexcept : v(o.v) { ++live; }
  Tracked(const Tracked&) = delete;
  ~Tracked() { --live; }
  int operator()(int x) { return v + x; }
};
int twice(int x) { return 2 * x; }

using M = std::move_only_function<int(int)>;
static_assert(std::is_same_v<M::result_type, int>);
static_assert(std::is_same_v<std::move_only_function<void() const noexcept>::result_type, void>);
static_assert(std::is_nothrow_default_constructible_v<M>);
static_assert(std::is_nothrow_constructible_v<M, std::nullptr_t>);
static_assert(std::is_nothrow_move_constructible_v<M>);
static_assert(!std::is_copy_constructible_v<M>);
static_assert(!std::is_copy_assignable_v<M>);
static_assert(std::is_move_assignable_v<M>);
static_assert(std::is_nothrow_assignable_v<M&, std::nullptr_t>);
static_assert(noexcept(std::declval<M&>().swap(std::declval<M&>())));
static_assert(noexcept(swap(std::declval<M&>(), std::declval<M&>())));
static_assert(noexcept(static_cast<bool>(std::declval<const M&>())));
static_assert(!std::is_convertible_v<M, bool>);
static_assert(noexcept(std::declval<const M&>() == nullptr));
static_assert(std::is_same_v<decltype(std::declval<const M&>() == nullptr), bool>);
static_assert(std::is_constructible_v<M, Tracked>);  // move-only targets are fine
static_assert(std::is_convertible_v<Tracked, M>);   // implicit converting constructor
static_assert(std::is_nothrow_destructible_v<M>);

int main() {
  M empty;
  CHECK(!empty);
  CHECK(empty == nullptr && nullptr == empty);
  CHECK(!(empty != nullptr));
  M n(nullptr);
  CHECK(!n);

  {
    M f = Tracked(5);
    CHECK(Tracked::live == 1);
    CHECK(f && f(1) == 6);
    CHECK(f != nullptr);
    M g = std::move(f);
    CHECK(g(2) == 7);
    CHECK(Tracked::live == 1);  // ownership transferred, no extra target
    g = nullptr;
    CHECK(!g && Tracked::live == 0);
    g = Tracked(1);
    CHECK(Tracked::live == 1);
    g = twice;  // previous target destroyed
    CHECK(Tracked::live == 0 && g(4) == 8);
    M& self = (g = Tracked(3));
    CHECK(&self == &g && g(0) == 3);
    M h;
    h = std::move(g);
    CHECK(h(1) == 4 && Tracked::live == 1);
  }
  CHECK(Tracked::live == 0);  // destructor destroyed the target

  M a = twice, b = Tracked(10);
  a.swap(b);
  CHECK(a(1) == 11 && b(1) == 2);
  swap(a, b);
  CHECK(a(1) == 2 && b(1) == 11);
  M c;
  c.swap(a);
  CHECK(!a && c(3) == 6);

  // lambdas with move-only captures
  struct Box {
    int v;
    Box(int x) : v(x) {}
    Box(Box&&) = default;
  };
  std::move_only_function<int()> l = [b = Box(42)] { return b.v; };
  CHECK(l() == 42);
  return 0;
}
