// [any.class.general]/3: "any such small-object optimization shall only be applied to types T
// for which is_nothrow_move_constructible_v<T> is true." Together with [any.cons]/4 (the move
// constructor is noexcept and "contains either the contained value of other, or ... an object
// ... constructed from the contained value of other considering that contained value as an
// rvalue"), a small type whose move constructor may throw must be held out of line: moving
// or swapping the any must not invoke (and so must not risk throwing from) its move
// constructor, and the contained object keeps its address.
#include <any>
#include <utility>
#include "check.hpp"

struct ThrowingMove {
  static inline int moves = 0;
  int v;
  ThrowingMove(int x) : v(x) {}
  ThrowingMove(const ThrowingMove& o) : v(o.v) {}
  ThrowingMove(ThrowingMove&& o) noexcept(false) : v(o.v) {
    ++moves;
    throw 1;  // never reached unless the any moves the value in place
  }
};

int main() {
  std::any a(std::in_place_type<ThrowingMove>, 5);
  ThrowingMove* p = std::any_cast<ThrowingMove>(&a);
  CHECK(p != nullptr);

  std::any b(std::move(a));
  CHECK(ThrowingMove::moves == 0);
  CHECK(std::any_cast<ThrowingMove>(&b) == p);
  CHECK(p->v == 5);

  std::any c = 1;
  c.swap(b);
  CHECK(ThrowingMove::moves == 0);
  CHECK(std::any_cast<ThrowingMove>(&c) == p);

  std::any d;
  d = std::move(c);
  CHECK(ThrowingMove::moves == 0);
  CHECK(std::any_cast<ThrowingMove>(&d) == p);
  return 0;
}
