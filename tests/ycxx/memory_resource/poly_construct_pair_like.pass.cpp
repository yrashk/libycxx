// [mem.poly.allocator.mem]/14: polymorphic_allocator::construct(p, args...) uses
// uses_allocator_construction_args<T> ([allocator.uses.construction]); for T a pair:
// /17-18 (P2165): a pair-like argument P other than a subrange (tuple<int,int>,
// array<int,2>) is split with get<0>/get<1> and each member is uses-allocator constructed;
// /19-22: a U that is not pair-like and not a pair but converts to the pair (or a
// ranges::subrange) is passed as a pair-constructor whose conversion uses-allocator constructs
// the pair from the converted value, so the members still receive the allocator.
// REQUIRES: exceptions
#include <memory_resource>
#include <array>
#include <cstddef>
#include <limits>
#include <new>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include "check.hpp"
#include "recording_resource.hpp"

using PA = std::pmr::polymorphic_allocator<>;

struct Leading {  // leading-allocator convention
  using allocator_type = PA;
  int v = 0;
  std::pmr::memory_resource* r = nullptr;
  Leading(std::allocator_arg_t, const PA& a) : r(a.resource()) {}
  Leading(std::allocator_arg_t, const PA& a, int x) : v(x), r(a.resource()) {}
  Leading(std::allocator_arg_t, const PA& a, const Leading& o) : v(o.v), r(a.resource()) {}
  Leading(std::allocator_arg_t, const PA& a, Leading&& o) : v(o.v + 1000), r(a.resource()) {}
  Leading(int x) : v(x) {}
  Leading(const Leading& o) : v(o.v) {}
  Leading(Leading&& o) : v(o.v + 1000) {}
};

struct Trailing {  // trailing-allocator convention
  using allocator_type = PA;
  int v = 0;
  std::pmr::memory_resource* r = nullptr;
  Trailing(const PA& a) : r(a.resource()) {}
  Trailing(int x, const PA& a) : v(x), r(a.resource()) {}
  Trailing(const Trailing& o, const PA& a) : v(o.v), r(a.resource()) {}
  Trailing(Trailing&& o, const PA& a) : v(o.v + 1000), r(a.resource()) {}
  Trailing(int x) : v(x) {}
  Trailing(const Trailing& o) : v(o.v) {}
  Trailing(Trailing&& o) : v(o.v + 1000) {}
};

using P = std::pair<Leading, Trailing>;

struct ToPair {  // not pair-like, converts to P
  int a, b;
  operator P() const { return P(Leading(a), Trailing(b)); }
};

template <class T, class... Args>
T* make(PA a, Args&&... args) {
  T* p = a.allocate_object<T>();
  a.construct(p, std::forward<Args>(args)...);
  return p;
}
template <class T>
void drop(PA a, T* p) {
  p->~T();
  a.deallocate_object(p);
}

int main() {
  RecordingResource r;
  PA a(&r);
  auto uses = [&](const P& p) { return p.first.r == &r && p.second.r == &r; };
  P* p = nullptr;
  p = make<P>(a, std::tuple<int, int>(9, 10));  // /17-18, pair-like tuple
  CHECK(uses(*p) && p->first.v == 9 && p->second.v == 10);
  drop(a, p);
  std::array<int, 2> arr{11, 12};
  p = make<P>(a, arr);  // pair-like array lvalue
  CHECK(uses(*p) && p->first.v == 11 && p->second.v == 12);
  drop(a, p);

  p = make<P>(a, std::piecewise_construct, std::forward_as_tuple(13), std::forward_as_tuple());
  CHECK(uses(*p) && p->first.v == 13 && p->second.v == 0);
  drop(a, p);

  // /19-22: converted, then uses-allocator constructed from the converted rvalue pair.
  p = make<P>(a, ToPair{14, 15});
  CHECK(uses(*p) && p->first.v % 1000 == 14 && p->second.v % 1000 == 15);
  drop(a, p);

  // A subrange converts to a pair (through /19-22, not /17-18).
  int xs[3] = {1, 2, 3};
  using IP = std::pair<int*, int*>;
  IP* ip = make<IP>(a, std::ranges::subrange<int*>(xs, xs + 3));
  CHECK(ip->first == xs && ip->second == xs + 3);
  drop(a, ip);

  using PP = std::pair<P, std::pair<Trailing, Leading>>;
  PP* pp = make<PP>(a, std::tuple<int, int>(1, 2), std::array<int, 2>{3, 4});
  CHECK(uses(pp->first) && pp->second.first.r == &r && pp->second.second.r == &r);
  CHECK(pp->first.first.v == 1 && pp->first.second.v == 2 && pp->second.first.v == 3 && pp->second.second.v == 4);
  drop(a, pp);
  CHECK(r.outstanding == 0);
  return 0;
}
