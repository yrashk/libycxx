// optional's in_place constructors and emplace construct the contained value in place from the
// forwarded arguments: no temporary T, no copy or move, the arguments' value categories kept.
//   [optional.ctor]/23: optional(in_place_t, Args&&... args): "Effects: Direct-non-list-
//     initializes val with std::forward<Args>(args)..."; /27: the initializer_list form "with
//     il, std::forward<Args>(args)..."; both constexpr if T's selected constructor is.
//   [optional.assign]: emplace(args...): "Effects: Calls *this = nullopt. Then direct-non-list-
//     initializes val with std::forward<Args>(args)..."; Returns: a reference to the new
//     contained value.
//   [optional.specalg] make_optional<T>(args...): "Returns: optional<T>(in_place,
//     std::forward<Args>(args)...)" (a prvalue, so T need not be movable).
// Pinned (support/inplace_probe.hpp) can be neither copied nor moved.
#include <initializer_list>
#include <optional>
#include <utility>
#include "check.hpp"
#include "inplace_probe.hpp"

using probe::Arg;
using probe::counts;
using probe::Pinned;
using probe::Probe;

struct ListPinned {
  int size = 0, cat = 0;
  ListPinned(std::initializer_list<int> il, Arg&) : size(static_cast<int>(il.size())), cat(probe::lref) {}
  ListPinned(std::initializer_list<int> il, Arg&& a) : size(static_cast<int>(il.size())), cat(probe::rref) {
    a.moved_from = true;
  }
  ListPinned(const ListPinned&) = delete;
};

// A literal type that can be neither copied nor moved.
struct Lit {
  int v, cat;
  constexpr Lit(int& x) : v(x), cat(probe::lref) {}
  constexpr Lit(int&& x) : v(x), cat(probe::rref) {}
  constexpr Lit(std::initializer_list<int> il, const int& x)
      : v(static_cast<int>(il.size()) + x), cat(probe::clref) {}
  Lit(const Lit&) = delete;
};

constexpr bool constant() {
  int x = 3;
  std::optional<Lit> a(std::in_place, x);
  std::optional<Lit> b(std::in_place, 4);
  std::optional<Lit> c(std::in_place, {1, 2}, x);
  bool ok = a->cat == probe::lref && b->cat == probe::rref && c->cat == probe::clref && c->v == 5;
  Lit& r = b.emplace(x);
  ok = ok && &r == &*b && b->cat == probe::lref && b->v == 3;
  Lit& s = a.emplace({1, 2, 3}, 7);
  return ok && &s == &*a && a->v == 10;
}
static_assert(constant());

int main() {
  Arg a{7};
  const Arg ca{8};
  probe::reset();
  {
    std::optional<Pinned> o1(std::in_place, 1, a);
    std::optional<Pinned> o2(std::in_place, 2, ca);
    std::optional<Pinned> o3(std::in_place, 3, std::move(ca));
    std::optional<Pinned> o4(std::in_place);
    CHECK(o1->cat == probe::lref && o2->cat == probe::clref && o3->cat == probe::crref);
    CHECK(o4.has_value() && o4->key == 0 && o4->cat == probe::none);
    CHECK(counts.made == 4 && counts.extra() == 0 && counts.destroyed == 0);

    // emplace on an engaged optional destroys the old value first, then constructs in place.
    Pinned* where = &*o1;
    Pinned& r = o1.emplace(9, std::move(a));
    CHECK(&r == where && r.key == 9 && r.cat == probe::rref && a.moved_from);
    CHECK(counts.made == 5 && counts.destroyed == 1 && counts.extra() == 0);
    std::optional<Pinned> e;
    Pinned& r2 = e.emplace(10, ca);
    CHECK(&r2 == &*e && r2.cat == probe::clref && counts.made == 6 && counts.destroyed == 1);
  }
  CHECK(counts.destroyed == 6 && counts.extra() == 0);

  // initializer_list forms.
  {
    Arg b{1};
    std::optional<ListPinned> l(std::in_place, {1, 2, 3}, b);
    CHECK(l->size == 3 && l->cat == probe::lref);
    l.emplace({4, 5}, std::move(b));
    CHECK(l->size == 2 && l->cat == probe::rref && b.moved_from);
  }

  // make_optional<T>(args...): no copy or move of the value itself.
  probe::reset();
  {
    Arg c{2};
    auto m = std::make_optional<Probe>(5, c);
    CHECK(m->cat == probe::lref && counts.made == 1 && counts.extra() == 0);
    auto n = std::make_optional<Pinned>(6, std::move(c));
    CHECK(n->cat == probe::rref && counts.made == 2 && counts.extra() == 0);
  }
  return 0;
}
