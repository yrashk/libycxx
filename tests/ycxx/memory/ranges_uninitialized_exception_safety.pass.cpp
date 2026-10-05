// [specialized.algorithms.general]/2: "Unless otherwise specified, if an exception is thrown
// in the following algorithms, objects constructed by a placement new-expression are destroyed
// in an unspecified order before allowing the exception to propagate." This covers the
// std::ranges forms too: ranges::uninitialized_default_construct(_n),
// uninitialized_value_construct(_n), uninitialized_copy(_n), uninitialized_move(_n) and
// uninitialized_fill(_n), in both their iterator-sentinel and range overloads. Throwing on
// the first element constructs nothing; the exception propagates unchanged.
// REQUIRES: exceptions
#include <memory>
#include <span>
#include "check.hpp"

struct Bomb {
  static inline int live = 0;
  static inline int countdown = -1;  // throw when it reaches 0
  int v = 0;
  static void tick() {
    if (countdown >= 0 && countdown-- == 0) throw 42;
  }
  Bomb() {
    tick();
    ++live;
  }
  Bomb(int x) : v(x) {
    tick();
    ++live;
  }
  Bomb(const Bomb& o) : v(o.v) {
    tick();
    ++live;
  }
  Bomb(Bomb&& o) : v(o.v) {
    tick();
    ++live;
  }
  ~Bomb() { --live; }
};

template <class F>
bool throws_and_cleans_up(F f, int after) {
  int base = Bomb::live;
  Bomb::countdown = after;
  bool threw = false;
  try {
    f();
  } catch (int e) {
    threw = e == 42;
  }
  Bomb::countdown = -1;
  return threw && Bomb::live == base;
}

int main() {
  namespace R = std::ranges;
  std::allocator<Bomb> a;
  Bomb* p = a.allocate(6);
  std::span<Bomb> out(p, 6);
  {
    Bomb src[6] = {1, 2, 3, 4, 5, 6};
    std::span<Bomb> in(src, 6);
    for (int k : {0, 1, 3, 5}) {
      CHECK(throws_and_cleans_up([&] { R::uninitialized_default_construct(p, p + 6); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_default_construct(out); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_default_construct_n(p, 6); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_value_construct(p, p + 6); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_value_construct(out); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_value_construct_n(p, 6); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_copy(src, src + 6, p, p + 6); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_copy(in, out); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_copy_n(src, 6, p, p + 6); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_move(src, src + 6, p, p + 6); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_move(in, out); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_move_n(src, 6, p, p + 6); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_fill(p, p + 6, src[0]); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_fill(out, src[1]); }, k));
      CHECK(throws_and_cleans_up([&] { R::uninitialized_fill_n(p, 6, src[2]); }, k));
    }
    CHECK(Bomb::live == 6);
    // without exceptions everything is constructed and can be destroyed again
    R::uninitialized_copy(in, out);
    CHECK(Bomb::live == 12 && p[5].v == 6);
    R::destroy(out);
    CHECK(Bomb::live == 6);
  }
  CHECK(Bomb::live == 0);
  a.deallocate(p, 6);
  return 0;
}
