// any's in_place_type constructors, emplace and make_any construct the contained value from the
// forwarded arguments, without a temporary that is then copied or moved into the any.
//   [any.cons]: any(in_place_type_t<T>, Args&&... args): "Effects: Direct-non-list-
//     initializes the contained value of type VT with std::forward<Args>(args)..."; the
//     initializer_list form "with il, std::forward<Args>(args)...".
//   [any.modifiers]: emplace<T>(args...): "Calls reset(). Then direct-non-list-initializes
//     the contained value of type VT with std::forward<Args>(args)..."; Returns: a reference to
//     the new contained value.
//   [any.nonmembers]: make_any<T>(args...): "Returns: any(in_place_type<T>,
//     std::forward<Args>(args)...)".
// VT must be copy constructible ([any.class]), so the probe is copyable, and counts that it was
// never copied or moved; this holds for small and for large contained values alike.
#include <any>
#include <initializer_list>
#include <utility>
#include "check.hpp"
#include "inplace_probe.hpp"

using probe::Arg;
using probe::counts;
using probe::Probe;

struct Big : Probe {
  using Probe::Probe;
  char pad[256] = {};
};

struct ListProbe {
  int size = 0, cat = 0;
  ListProbe(std::initializer_list<int> il, Arg&) : size(static_cast<int>(il.size())), cat(probe::lref) {}
  ListProbe(std::initializer_list<int> il, Arg&&) : size(static_cast<int>(il.size())), cat(probe::rref) {}
  ListProbe(const ListProbe& o) : size(o.size), cat(o.cat) { ++counts.copies; }
  ListProbe(ListProbe&& o) noexcept : size(o.size), cat(o.cat) { ++counts.moves; }
};

template <class T>
void run() {
  Arg a{7};
  const Arg ca{8};
  probe::reset();
  {
    std::any x(std::in_place_type<T>, 1, a);
    std::any y(std::in_place_type<T>, 2, std::move(ca));
    CHECK(std::any_cast<T&>(x).cat == probe::lref && std::any_cast<T&>(y).cat == probe::crref);
    CHECK(counts.made == 2 && counts.extra() == 0 && counts.destroyed == 0);

    T& r = x.emplace<T>(3, ca);
    CHECK(&r == std::any_cast<T>(&x) && r.cat == probe::clref);
    CHECK(counts.made == 3 && counts.destroyed == 1 && counts.extra() == 0);

    std::any z = std::make_any<T>(4, std::move(a));
    CHECK(std::any_cast<T&>(z).cat == probe::rref && a.moved_from);
    CHECK(counts.made == 4 && counts.extra() == 0);
  }
  CHECK(counts.destroyed == 4);
}

int main() {
  run<Probe>();
  run<Big>();
  probe::reset();
  Arg b{1};
  std::any l(std::in_place_type<ListProbe>, {1, 2}, b);
  CHECK(std::any_cast<ListProbe&>(l).size == 2 && std::any_cast<ListProbe&>(l).cat == probe::lref);
  l.emplace<ListProbe>({1, 2, 3}, Arg{});
  CHECK(std::any_cast<ListProbe&>(l).size == 3 && std::any_cast<ListProbe&>(l).cat == probe::rref);
  std::any m = std::make_any<ListProbe>({4}, b);
  CHECK(std::any_cast<ListProbe&>(m).size == 1);
  CHECK(counts.extra() == 0);
  return 0;
}
