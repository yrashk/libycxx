// The in_place_type constructors of move_only_function and copyable_function create the target
// object directly from the forwarded arguments: the target is never a temporary that is then
// copied or moved into the wrapper. (std::function has no in_place_type constructor.)
//   [func.wrap.move.ctor]/10-14: move_only_function(in_place_type_t<T>, Args&&... args):
//     Constraints: is_constructible_v<VT, Args...> and is-callable-from<VT>; Preconditions: VT
//     meets the Cpp17Destructible requirements, "and if is_move_constructible_v<VT> is true,
//     VT meets the Cpp17MoveConstructible requirements" (so VT need not be movable);
//     Postconditions: "*this has a target object of type VT direct-non-list-initialized with
//     std::forward<Args>(args)..."; /16-20 the initializer_list form.
//   [func.wrap.move.ctor]/3: move_only_function(move_only_function&& f) noexcept:
//     "Postconditions: The target object of *this is the target object f had before
//     construction" - for a target that cannot be moved, the very same object; swap and move
//     assignment are defined in terms of it ([func.wrap.move.ctor] operator=(&&): "Equivalent
//     to: move_only_function(std::move(f)).swap(*this);").
//   [func.wrap.copy.ctor]/12-16: copyable_function(in_place_type_t<T>, Args&&... args): the same
//     postcondition (VT must be Cpp17CopyConstructible).
// Small and large targets (the wrappers may store small ones inline, [func.wrap.move.class]/4)
// are both checked.
#include <functional>
#include <initializer_list>
#include <utility>
#include "check.hpp"
#include "inplace_probe.hpp"

using probe::Arg;
using probe::counts;

template <bool Movable, int Pad>
struct Fn : probe::Basic<Movable> {
  using probe::Basic<Movable>::Basic;
  const void* self = this;
  char pad[Pad] = {};
  Fn(std::initializer_list<int> il, Arg& a) : probe::Basic<Movable>(static_cast<int>(il.size()), a) {}
  int operator()(int x) const { return this->key * 100 + this->cat * 10 + x; }
  bool unmoved() const { return self == this; }
};

// Reports whether its pinned member is still the object constructed first.
struct Checker {
  Fn<false, 1> inner;
  Checker(int k, Arg& x) : inner(k, x) {}
  Checker(const Checker&) = delete;
  bool operator()() const { return inner.unmoved(); }
};

template <class F>
void move_only_cases() {
  Arg a{1};
  const Arg ca{2};
  probe::reset();
  {
    std::move_only_function<int(int) const> f(std::in_place_type<F>, 1, a);
    std::move_only_function<int(int) const> g(std::in_place_type<F>, 2, std::move(ca));
    std::move_only_function<int(int) const> h(std::in_place_type<F>, 3, std::move(a));
    std::move_only_function<int(int) const> l(std::in_place_type<F>, {4, 5}, a);
    CHECK(f(0) == 100 + 10 * probe::lref && g(1) == 200 + 10 * probe::crref + 1);
    CHECK(h(0) == 300 + 10 * probe::rref && a.moved_from);
    CHECK(l(0) == 200 + 10 * probe::lref);
    CHECK(counts.made == 4 && counts.extra() == 0 && counts.destroyed == 0);

    // Moving and swapping the wrappers keeps working; a target that cannot be moved stays the
    // same object.
    std::move_only_function<int(int) const> m(std::move(f));
    CHECK(m(0) == 100 + 10 * probe::lref);
    m.swap(g);
    CHECK(m(0) == 200 + 10 * probe::crref && g(0) == 100 + 10 * probe::lref);
    h = std::move(m);
    CHECK(h(0) == 200 + 10 * probe::crref);
    CHECK(counts.made == 4 && counts.copies == 0 && counts.copy_assigns == 0 && counts.move_assigns == 0);
    if constexpr (!std::is_move_constructible_v<F>) {
      CHECK(counts.moves == 0);
      CHECK(counts.destroyed == 1);  // pinned h's old target
    } else {
      CHECK(counts.destroyed >= 1);
      CHECK(counts.destroyed <= counts.made + counts.moves);
    }
  }
  CHECK(counts.destroyed == counts.made + counts.copies + counts.moves);
}

template <class F>
void copyable_cases() {
  Arg a{1};
  const Arg ca{2};
  probe::reset();
  {
    std::copyable_function<int(int) const> f(std::in_place_type<F>, 1, a);
    std::copyable_function<int(int) const> g(std::in_place_type<F>, 2, ca);
    std::copyable_function<int(int) const> h(std::in_place_type<F>, 3, Arg{});
    std::copyable_function<int(int) const> l(std::in_place_type<F>, {4, 5, 6}, a);
    CHECK(f(0) == 100 + 10 * probe::lref && g(0) == 200 + 10 * probe::clref);
    CHECK(h(0) == 300 + 10 * probe::rref && l(0) == 300 + 10 * probe::lref);
    CHECK(counts.made == 4 && counts.extra() == 0 && counts.destroyed == 0);
  }
  CHECK(counts.destroyed == 4);
}

int main() {
  move_only_cases<Fn<false, 1>>();
  move_only_cases<Fn<false, 512>>();
  move_only_cases<Fn<true, 1>>();
  move_only_cases<Fn<true, 512>>();
  copyable_cases<Fn<true, 1>>();
  copyable_cases<Fn<true, 512>>();

  // A pinned target is the same object after the wrapper is moved, move-assigned or swapped.
  {
    Arg a{0};
    std::move_only_function<bool() const> f(std::in_place_type<Checker>, 1, a);
    std::move_only_function<bool() const> c(std::in_place_type<Checker>, 2, a);
    CHECK(c());
    std::move_only_function<bool() const> d(std::move(c));
    CHECK(d());
    std::move_only_function<bool() const> e;
    e = std::move(d);
    CHECK(e());
    e.swap(f);
    CHECK(f() && e());
  }
  return 0;
}
