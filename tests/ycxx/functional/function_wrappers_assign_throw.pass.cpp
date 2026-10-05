// Assignments are specified as construct-then-swap, so a throwing construction leaves the
// assigned-to wrapper untouched:
// [func.wrap.func.con]/24 operator=(F&&): "function(std::forward<F>(f)).swap(*this);"; /19
// operator=(const function&): "function(f).swap(*this);".
// [func.wrap.move.ctor]/26: "move_only_function(std::forward<F>(f)).swap(*this);".
// [func.wrap.copy.ctor]/24: "copyable_function(f).swap(*this);"; /30 likewise for F&&.
// The constructors' Throws: clauses ("Any exception thrown by the initialization of the target
// object") let the exception propagate.
// REQUIRES: exceptions
#include <functional>
#include <utility>
#include "check.hpp"

struct Bomb {
  static inline bool armed = false;
  int v;
  explicit Bomb(int x) : v(x) {}
  Bomb(const Bomb& o) : v(o.v) {
    if (armed) throw 1;
  }
  Bomb(Bomb&& o) : v(o.v) {
    if (armed) throw 2;
  }
  int operator()() const { return v; }
};

template <class F>
bool throws(F f) {
  try {
    f();
  } catch (int) {
    return true;
  }
  return false;
}

template <class W>
void exercise() {
  Bomb::armed = false;
  W target = [] { return 7; };
  W loaded = Bomb(3);
  Bomb b(4);
  Bomb::armed = true;
  CHECK(throws([&] { target = b; }));
  CHECK(target() == 7);
  CHECK(throws([&] { target = std::move(b); }));
  CHECK(target() == 7);
  CHECK(throws([&] { W w(std::in_place_type<Bomb>, b); }));
  if constexpr (std::is_copy_constructible_v<W>) {
    CHECK(throws([&] { target = loaded; }));  // copying the Bomb target throws
    CHECK(target() == 7);
    CHECK(loaded() == 3);
  }
  Bomb::armed = false;
  target = b;
  CHECK(target() == 4);
}

int main() {
  exercise<std::move_only_function<int()>>();
  exercise<std::copyable_function<int()>>();
  // std::function has no in_place_type constructor; exercise it separately
  Bomb::armed = false;
  std::function<int()> target = [] { return 7; };
  std::function<int()> loaded = Bomb(3);
  Bomb b(4);
  Bomb::armed = true;
  CHECK(throws([&] { target = b; }));
  CHECK(target() == 7);
  CHECK(throws([&] { target = loaded; }));
  CHECK(target() == 7 && loaded() == 3);
  Bomb::armed = false;
  return 0;
}
