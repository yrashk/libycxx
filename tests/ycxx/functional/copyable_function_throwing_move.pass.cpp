// [func.wrap.copy.ctor]/5: "copyable_function(copyable_function&& f) noexcept;
// Postconditions: The target object of *this is the target object f had before construction".
// /26: operator=(copyable_function&& f) "Equivalent to:
// copyable_function(std::move(f)).swap(*this);" [func.wrap.copy.util]/1: swap is noexcept and
// "Exchanges the target objects". /3: the copy constructor copies the target. So a target whose
// move constructor throws is never moved: moving, move-assigning and swapping keep the same
// target objects, and only copying invokes the target's copy constructor.
// REQUIRES: exceptions
#include <functional>
#include <cstdlib>
#include <exception>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct ThrowingMove {
  int v;
  explicit ThrowingMove(int x) : v(x) {}
  ThrowingMove(const ThrowingMove& o) : v(o.v + 100) {}
  ThrowingMove(ThrowingMove&&) { throw 1; }
  const void* operator()() const { return this; }
  int value() const { return v; }
};

using C = std::copyable_function<const void*() const>;
static_assert(std::is_nothrow_move_constructible_v<C>);
static_assert(std::is_nothrow_swappable_v<C>);

int main() {
  std::set_terminate([] { std::_Exit(3); });
  C a(std::in_place_type<ThrowingMove>, 1);
  const void* addr = a();
  C b = std::move(a);  // noexcept: the target is not moved
  CHECK(b() == addr);
  C c(std::in_place_type<ThrowingMove>, 2);
  const void* caddr = c();
  b.swap(c);
  CHECK(b() == caddr && c() == addr);
  swap(b, c);
  CHECK(b() == addr && c() == caddr);
  C d;
  d = std::move(b);
  CHECK(d() == addr);

  // copying uses the copy constructor and yields a distinct target
  C e = d;
  CHECK(e() != addr && d() == addr);
  C f(std::in_place_type<ThrowingMove>, 3);
  f = d;  // copyable_function(d).swap(*this)
  CHECK(f() != addr);
  // from an lvalue target: copied, never moved
  const ThrowingMove proto(5);
  C g = proto;
  CHECK(static_cast<const ThrowingMove*>(g())->value() == 105);
  C h = std::move(g);
  CHECK(static_cast<const ThrowingMove*>(h())->value() == 105);
  return 0;
}
