// [specialized.algorithms.general]/2: "Unless otherwise specified, if an exception is thrown
// in the following algorithms, objects constructed by a placement new-expression are destroyed
// in an unspecified order before allowing the exception to propagate."
// REQUIRES: exceptions
#include <memory>
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
  std::allocator<Bomb> a;
  Bomb* p = a.allocate(5);
  {
    Bomb::countdown = -1;
    Bomb src[5] = {1, 2, 3, 4, 5};
    CHECK(throws_and_cleans_up([&] { std::uninitialized_default_construct(p, p + 5); }, 3));
    CHECK(throws_and_cleans_up([&] { std::uninitialized_default_construct_n(p, 5); }, 2));
    CHECK(throws_and_cleans_up([&] { std::uninitialized_value_construct(p, p + 5); }, 4));
    CHECK(throws_and_cleans_up([&] { std::uninitialized_value_construct_n(p, 5); }, 1));
    CHECK(throws_and_cleans_up([&] { std::uninitialized_copy(src, src + 5, p); }, 3));
    CHECK(throws_and_cleans_up([&] { std::uninitialized_copy_n(src, 5, p); }, 2));
    CHECK(throws_and_cleans_up([&] { std::uninitialized_move(src, src + 5, p); }, 3));
    CHECK(throws_and_cleans_up([&] { std::uninitialized_move_n(src, 5, p); }, 4));
    CHECK(throws_and_cleans_up([&] { std::uninitialized_fill(p, p + 5, src[0]); }, 2));
    CHECK(throws_and_cleans_up([&] { std::uninitialized_fill_n(p, 5, src[0]); }, 3));
    // throwing on the very first element constructs nothing
    CHECK(throws_and_cleans_up([&] { std::uninitialized_copy(src, src + 5, p); }, 0));
    CHECK(Bomb::live == 5);
  }
  CHECK(Bomb::live == 0);
  a.deallocate(p, 5);
  return 0;
}
