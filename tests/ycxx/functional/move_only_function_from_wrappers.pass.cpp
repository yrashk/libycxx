// [func.wrap.move.ctor]/8: "Postconditions: *this has no target object if ... remove_cvref_t<F>
// is a specialization of the move_only_function or copyable_function class template, and f has
// no target object. Otherwise, *this has a target object of type VT direct-non-list-initialized
// with std::forward<F>(f)." So: an empty std::function is NOT in that list and becomes a target
// (calling it throws bad_function_call, [func.wrap.func.inv]/2); a non-empty
// move_only_function / copyable_function source has its target moved or copied along with its
// state (whether or not the implementation avoids double wrapping); a reference_wrapper target
// refers to the original object. /14: in_place_type construction. /28: the destructor destroys
// the target.
// REQUIRES: exceptions
#include <functional>
#include <utility>
#include "check.hpp"

struct Counter {
  int n = 0;
  int operator()() { return ++n; }
  int operator()() const { return n; }
};
struct Tracked {
  static inline int live = 0;
  static inline int moves = 0;
  int v;
  explicit Tracked(int x) : v(x) { ++live; }
  Tracked(Tracked&& o) noexcept : v(o.v) {
    ++live;
    ++moves;
  }
  Tracked(const Tracked& o) : v(o.v) { ++live; }
  ~Tracked() { --live; }
  int operator()() const noexcept { return v; }
};

int main() {
  // an empty std::function is a target object
  std::move_only_function<int()> e = std::function<int()>();
  CHECK(static_cast<bool>(e) && e != nullptr);
  bool threw = false;
  try {
    e();
  } catch (const std::bad_function_call&) {
    threw = true;
  }
  CHECK(threw);

  // from a copyable_function rvalue: the target, with its state, is taken over
  std::copyable_function<int()> cf = Counter{};
  cf();
  cf();
  std::move_only_function<int()> m1 = std::move(cf);
  CHECK(m1() == 3);
  // from a copyable_function lvalue: the target is copied, the source is independent
  std::copyable_function<int()> cf2 = Counter{};
  cf2();
  std::move_only_function<int()> m2 = cf2;
  CHECK(m2() == 2 && m2() == 3);
  CHECK(cf2() == 2);
  // across signatures (const -> non-const, noexcept -> not, R conversion)
  {
    std::move_only_function<int() const noexcept> src(std::in_place_type<Tracked>, 7);
    CHECK(Tracked::live == 1);
    std::move_only_function<long()> dst = std::move(src);
    CHECK(dst() == 7L);
    CHECK(Tracked::live == 1);  // no copy of the target object was made
    std::move_only_function<void() &&> d2 = std::move(dst);
    std::move(d2)();
    CHECK(Tracked::live == 1);
    d2 = nullptr;
    CHECK(Tracked::live == 0);  // destroying the outer wrapper destroys the target
  }
  {
    std::copyable_function<int() const> csrc(std::in_place_type<Tracked>, 3);
    std::move_only_function<int()> fromc = csrc;  // copies: two Tracked objects
    CHECK(Tracked::live == 2);
    CHECK(fromc() == 3 && csrc() == 3);
  }
  CHECK(Tracked::live == 0);

  // a reference_wrapper target refers to the original object
  Counter c;
  std::move_only_function<int()> r = std::ref(c);
  r();
  r();
  CHECK(c.n == 2);
  std::move_only_function<int() const> cr(std::in_place_type<std::reference_wrapper<const Counter>>, c);
  c();
  CHECK(cr() == 3);
  // in_place_type with a function pointer type
  std::move_only_function<int(int)> fp(std::in_place_type<int (*)(int)>, [](int v) { return v + 1; });
  CHECK(fp(1) == 2);
  return 0;
}
