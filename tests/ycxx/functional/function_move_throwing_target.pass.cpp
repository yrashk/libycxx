// [func.wrap.func.con]/6: "function(function&& f) noexcept; Postconditions: If !f, *this has no
// target; otherwise, the target of *this is equivalent to the target of f before the
// construction". [func.wrap.func.mod]/1: "void swap(function& other) noexcept; Effects:
// Interchanges the target objects of *this and other." [func.wrap.func.alg]: swap(f1, f2)
// "Effects: As if by: f1.swap(f2);". So a target whose move constructor throws (copying is
// fine, as Cpp17CopyConstructible requires) must never make these operations throw or call
// std::terminate.
#include <functional>
#include <cstdlib>
#include <exception>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct ThrowingMove {
  int v;
  int pad[8];  // big enough that nobody is tempted to keep it inline
  explicit ThrowingMove(int x) : v(x), pad{} {}
  ThrowingMove(const ThrowingMove& o) : v(o.v), pad{} {}
  ThrowingMove(ThrowingMove&&) { throw 99; }
  int operator()() const { return v; }
};
struct SmallThrowingMove {  // the same, but tiny
  int v;
  explicit SmallThrowingMove(int x) : v(x) {}
  SmallThrowingMove(const SmallThrowingMove& o) : v(o.v) {}
  SmallThrowingMove(SmallThrowingMove&&) { throw 98; }
  int operator()() const { return v; }
};

static_assert(!std::is_nothrow_move_constructible_v<ThrowingMove>);
static_assert(std::is_nothrow_move_constructible_v<std::function<int()>>);
static_assert(std::is_nothrow_swappable_v<std::function<int()>>);

int main() {
  std::set_terminate([] { std::_Exit(3); });
  const ThrowingMove proto(7);
  std::function<int()> f = proto;  // copies, never moves
  CHECK(f() == 7);
  std::function<int()> g = std::move(f);  // must not throw (noexcept)
  CHECK(g && g() == 7);
  CHECK(g.target<ThrowingMove>() != nullptr);

  const SmallThrowingMove sproto(5);
  std::function<int()> s = sproto;
  std::function<int()> t = std::move(s);
  CHECK(t() == 5);

  // swap of two non-empty wrappers with throwing-move targets, member and non-member
  t.swap(g);
  CHECK(t() == 7 && g() == 5);
  swap(t, g);
  CHECK(t() == 5 && g() == 7);
  // swap with an empty wrapper
  std::function<int()> e;
  e.swap(g);
  CHECK(!g && e() == 7);
  // copy still works through the copy constructor
  std::function<int()> c = e;
  CHECK(c() == 7 && e() == 7);
  return 0;
}
