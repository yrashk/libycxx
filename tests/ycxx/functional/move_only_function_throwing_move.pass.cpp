// [func.wrap.move.ctor]/3: "move_only_function(move_only_function&& f) noexcept;
// Postconditions: The target object of *this is the target object f had before construction".
// /22: operator=(move_only_function&& f) "Equivalent to:
// move_only_function(std::move(f)).swap(*this);" [func.wrap.move.util]/1: swap is noexcept and
// "Exchanges the target objects". /7: VT need only meet Cpp17MoveConstructible "if
// is_move_constructible_v<VT> is true". [func.wrap.move.class]/2 Note 1: "small-object
// optimization can only be applied to a type T for which is_nothrow_move_constructible_v<T> is
// true." So a target whose move constructor throws, or that cannot be moved at all, is never
// moved: moving, move-assigning and swapping the wrappers keep the very same target objects.
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
  ThrowingMove(ThrowingMove&&) { throw 1; }
  const ThrowingMove* self() const { return this; }
  int operator()() const { return v; }
};
struct Immovable {
  int v;
  explicit Immovable(int x) : v(x) {}
  Immovable(Immovable&&) = delete;
  int operator()() const { return v; }
};

using M = std::move_only_function<int() const>;
using P = std::move_only_function<const void*() const>;
static_assert(std::is_nothrow_move_constructible_v<M>);
static_assert(std::is_nothrow_swappable_v<M>);

int main() {
  std::set_terminate([] { std::_Exit(3); });
  // the target is constructed in place, so its throwing move constructor is never needed
  M a(std::in_place_type<ThrowingMove>, 4);
  CHECK(a() == 4);
  M b = std::move(a);
  CHECK(b() == 4);
  M c(std::in_place_type<ThrowingMove>, 6);
  b.swap(c);
  CHECK(b() == 6 && c() == 4);
  swap(b, c);
  CHECK(b() == 4 && c() == 6);
  c = std::move(b);  // construct + swap: nothing throws
  CHECK(c() == 4);
  M e;
  e = std::move(c);
  CHECK(e() == 4);

  // identity: the very same target object travels with the wrapper
  struct Where {
    ThrowingMove t;
    explicit Where(int x) : t(x) {}
    Where(Where&&) : t(0) { throw 2; }
    const void* operator()() const { return this; }
  };
  P p(std::in_place_type<Where>, 1);
  const void* addr = p();
  P q = std::move(p);
  CHECK(q() == addr);
  P r;
  r.swap(q);
  CHECK(r() == addr);
  P s;
  s = std::move(r);
  CHECK(s() == addr);

  struct ImmovableWhere {
    ImmovableWhere() = default;
    ImmovableWhere(ImmovableWhere&&) = delete;
    const void* operator()() const { return this; }
  };
  P i(std::in_place_type<ImmovableWhere>);
  const void* iaddr = i();
  P j = std::move(i);
  CHECK(j() == iaddr);
  swap(j, s);
  CHECK(s() == iaddr && j() == addr);

  M im(std::in_place_type<Immovable>, 9);
  M im2;
  im2 = std::move(im);
  CHECK(im2() == 9);
  return 0;
}
