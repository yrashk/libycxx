// [utility.exchange]: template<class T, class U = T> constexpr T exchange(T& obj, U&& new_val)
// noexcept(is_nothrow_move_constructible_v<T> && is_nothrow_assignable_v<T&, U>);
// "Effects: Equivalent to: T old_val = std::move(obj); obj = std::forward<U>(new_val);
// return old_val;"
#include <utility>
#include <type_traits>
#include "check.hpp"

struct Tracker {
  int v;
  int copies = 0, moves = 0;
  constexpr Tracker(int x) : v(x) {}
  constexpr Tracker(const Tracker& o) : v(o.v), copies(o.copies + 1), moves(o.moves) {}
  constexpr Tracker(Tracker&& o) noexcept : v(o.v), copies(o.copies), moves(o.moves + 1) {}
  constexpr Tracker& operator=(const Tracker& o) {
    v = o.v;
    copies = 100;
    return *this;
  }
  constexpr Tracker& operator=(Tracker&& o) noexcept {
    v = o.v;
    moves = 100;
    return *this;
  }
  constexpr Tracker& operator=(int x) noexcept {
    v = x;
    return *this;
  }
};
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(ThrowingMove&&) {}
  ThrowingMove& operator=(ThrowingMove&&) noexcept { return *this; }
};
struct ThrowingIntAssign {
  ThrowingIntAssign() = default;
  ThrowingIntAssign(ThrowingIntAssign&&) noexcept {}
  ThrowingIntAssign& operator=(ThrowingIntAssign&&) noexcept { return *this; }
  ThrowingIntAssign& operator=(int) { return *this; }
};
struct Pt {
  int x, y;
};

static_assert(noexcept(std::exchange(std::declval<int&>(), 1)));
static_assert(!noexcept(std::exchange(std::declval<ThrowingMove&>(), ThrowingMove{})));
static_assert(noexcept(std::exchange(std::declval<ThrowingIntAssign&>(), ThrowingIntAssign{})));
static_assert(!noexcept(std::exchange(std::declval<ThrowingIntAssign&>(), 1)));
static_assert(std::is_same_v<decltype(std::exchange(std::declval<long&>(), 1)), long>);

constexpr bool test() {
  int i = 1;
  if (std::exchange(i, 2) != 1 || i != 2) return false;
  // the old value is moved out, the new value is forwarded
  Tracker t(1);
  Tracker old = std::exchange(t, Tracker(2));
  if (old.v != 1 || t.v != 2) return false;
  if (old.moves < 1 || old.copies != 0) return false;
  if (t.moves != 100) return false;  // move-assigned from the rvalue
  Tracker src(3);
  (void)std::exchange(t, src);  // an lvalue is copy-assigned
  if (t.v != 3 || t.copies != 100) return false;
  (void)std::exchange(t, 9);  // heterogeneous U
  if (t.v != 9) return false;
  // U defaults to T, so a braced-init-list works
  Pt p{1, 2};
  Pt q = std::exchange(p, {3, 4});
  if (q.x != 1 || p.x != 3 || p.y != 4) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
