// [allocator.uses.construction]: uses_allocator_construction_args, make_obj_using_allocator
// and uninitialized_construct_using_allocator, including the pair overloads that apply
// uses-allocator construction to first and second individually.
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Alloc { int id; };

// Does not use an allocator.
struct Plain {
  int v;
  constexpr Plain() : v(-1) {}
  constexpr Plain(int x) : v(x) {}
};
// Leading-allocator convention.
struct Leading {
  using allocator_type = Alloc;
  int v, alloc_id;
  constexpr Leading() : v(-1), alloc_id(0) {}
  constexpr Leading(std::allocator_arg_t, const Alloc& a) : v(-1), alloc_id(a.id) {}
  constexpr Leading(std::allocator_arg_t, const Alloc& a, int x) : v(x), alloc_id(a.id) {}
  constexpr Leading(int x) : v(x), alloc_id(0) {}
  constexpr Leading(const Leading& o) : v(o.v), alloc_id(o.alloc_id) {}
  constexpr Leading(std::allocator_arg_t, const Alloc& a, const Leading& o) : v(o.v), alloc_id(a.id) {}
};
// Trailing-allocator convention.
struct Trailing {
  using allocator_type = Alloc;
  int v, alloc_id;
  constexpr Trailing(const Alloc& a) : v(-1), alloc_id(a.id) {}
  constexpr Trailing(int x, const Alloc& a) : v(x), alloc_id(a.id) {}
  constexpr Trailing(int x) : v(x), alloc_id(0) {}
};
// Both conventions: leading is preferred ([allocator.uses.construction]/2.2).
struct Both {
  using allocator_type = Alloc;
  int which;
  constexpr Both(std::allocator_arg_t, const Alloc&, int) : which(1) {}
  constexpr Both(int, const Alloc&) : which(2) {}
};
// Convertible to a pair but not pair-like ([allocator.uses.construction]/19-22).
struct ToPair {
  constexpr operator std::pair<Leading, Plain>() const { return {Leading(7), Plain(8)}; }
};

static_assert(std::uses_allocator_v<Leading, Alloc> && std::uses_allocator_v<Trailing, Alloc>);
static_assert(!std::uses_allocator_v<Plain, Alloc>);

// Return types (/5).
static_assert(std::is_same_v<decltype(std::uses_allocator_construction_args<Plain>(Alloc{}, 1)), std::tuple<int&&>>);
static_assert(std::is_same_v<decltype(std::uses_allocator_construction_args<Leading>(Alloc{}, 1)),
                             std::tuple<std::allocator_arg_t, const Alloc&, int&&>>);
static_assert(std::is_same_v<decltype(std::uses_allocator_construction_args<Trailing>(Alloc{}, 1)),
                             std::tuple<int&&, const Alloc&>>);
static_assert(std::is_same_v<decltype(std::uses_allocator_construction_args<const Trailing>(Alloc{}, std::declval<int&>())),
                             std::tuple<int&, const Alloc&>>);
static_assert(noexcept(std::uses_allocator_construction_args<Leading>(Alloc{}, 1)));
// Pair overloads return piecewise_construct plus two tuples (/8).
using P = std::pair<Leading, Trailing>;
static_assert(std::is_same_v<decltype(std::uses_allocator_construction_args<P>(Alloc{})),
                             std::tuple<std::piecewise_construct_t,
                                        std::tuple<std::allocator_arg_t, const Alloc&>,
                                        std::tuple<const Alloc&>>>);
static_assert(std::is_same_v<decltype(std::uses_allocator_construction_args<P>(Alloc{}, 1, 2)),
                             std::tuple<std::piecewise_construct_t,
                                        std::tuple<std::allocator_arg_t, const Alloc&, int&&>,
                                        std::tuple<int&&, const Alloc&>>>);
static_assert(std::is_same_v<decltype(std::uses_allocator_construction_args<P>(Alloc{}, std::declval<std::pair<int, int>&>())),
                             std::tuple<std::piecewise_construct_t,
                                        std::tuple<std::allocator_arg_t, const Alloc&, int&>,
                                        std::tuple<int&, const Alloc&>>>);
static_assert(std::is_same_v<decltype(std::uses_allocator_construction_args<P>(Alloc{}, std::declval<const std::pair<int, int>&>())),
                             std::tuple<std::piecewise_construct_t,
                                        std::tuple<std::allocator_arg_t, const Alloc&, const int&>,
                                        std::tuple<const int&, const Alloc&>>>);
static_assert(std::is_same_v<decltype(std::uses_allocator_construction_args<P>(Alloc{}, std::declval<std::pair<int, int>>())),
                             std::tuple<std::piecewise_construct_t,
                                        std::tuple<std::allocator_arg_t, const Alloc&, int&&>,
                                        std::tuple<int&&, const Alloc&>>>);
// pair-like (a two-element tuple) uses get<0>/get<1> (/17-18).
static_assert(std::is_same_v<decltype(std::uses_allocator_construction_args<P>(Alloc{}, std::declval<std::tuple<int, long>>())),
                             std::tuple<std::piecewise_construct_t,
                                        std::tuple<std::allocator_arg_t, const Alloc&, int&&>,
                                        std::tuple<long&&, const Alloc&>>>);
// Convertible-to-pair: a single-element tuple holding the pair-constructor (/22).
static_assert(std::tuple_size_v<decltype(std::uses_allocator_construction_args<std::pair<Leading, Plain>>(Alloc{}, ToPair{}))> == 1);

constexpr bool test() {
  const Alloc a{42};
  {
    Plain p = std::make_obj_using_allocator<Plain>(a, 3);
    if (p.v != 3) return false;
    Leading l = std::make_obj_using_allocator<Leading>(a, 4);
    if (l.v != 4 || l.alloc_id != 42) return false;
    Trailing t = std::make_obj_using_allocator<Trailing>(a, 5);
    if (t.v != 5 || t.alloc_id != 42) return false;
    Both b = std::make_obj_using_allocator<Both>(a, 6);
    if (b.which != 1) return false;
    Leading l0 = std::make_obj_using_allocator<Leading>(a);
    if (l0.v != -1 || l0.alloc_id != 42) return false;
  }
  {
    // pair: default, two arguments, piecewise.
    auto p0 = std::make_obj_using_allocator<std::pair<Leading, Trailing>>(a);
    if (p0.first.alloc_id != 42 || p0.second.alloc_id != 42 || p0.first.v != -1) return false;
    auto p1 = std::make_obj_using_allocator<std::pair<Leading, Plain>>(a, 1, 2);
    if (p1.first.v != 1 || p1.first.alloc_id != 42 || p1.second.v != 2) return false;
    auto p2 = std::make_obj_using_allocator<std::pair<Trailing, Leading>>(
        a, std::piecewise_construct, std::forward_as_tuple(3), std::forward_as_tuple(4));
    if (p2.first.v != 3 || p2.first.alloc_id != 42 || p2.second.v != 4 || p2.second.alloc_id != 42) return false;
  }
  {
    // pair from pair& / const pair& / pair&& / pair-like / convertible-to-pair.
    std::pair<int, int> src(5, 6);
    auto q1 = std::make_obj_using_allocator<std::pair<Leading, Trailing>>(a, src);
    auto q2 = std::make_obj_using_allocator<std::pair<Leading, Trailing>>(a, std::as_const(src));
    auto q3 = std::make_obj_using_allocator<std::pair<Leading, Trailing>>(a, std::move(src));
    auto q4 = std::make_obj_using_allocator<std::pair<Leading, Trailing>>(a, std::tuple<int, int>(7, 8));
    if (q1.first.v != 5 || q1.second.v != 6 || q1.second.alloc_id != 42) return false;
    if (q2.first.alloc_id != 42 || q3.second.alloc_id != 42) return false;
    if (q4.first.v != 7 || q4.second.v != 8 || q4.first.alloc_id != 42 || q4.second.alloc_id != 42) return false;
    auto q5 = std::make_obj_using_allocator<std::pair<Leading, Plain>>(a, ToPair{});
    if (q5.first.v != 7 || q5.first.alloc_id != 42 || q5.second.v != 8) return false;
  }
  {
    // Nested pair: the allocator reaches the inner pair's members.
    using Inner = std::pair<Leading, Trailing>;
    auto n = std::make_obj_using_allocator<std::pair<Inner, Plain>>(a, std::piecewise_construct,
                                                                     std::forward_as_tuple(1, 2), std::forward_as_tuple(3));
    if (n.first.first.alloc_id != 42 || n.first.second.alloc_id != 42 || n.first.second.v != 2) return false;
  }
  {
    // uninitialized_construct_using_allocator constructs in place and returns p.
    std::allocator<Leading> al;
    Leading* p = al.allocate(1);
    Leading* r = std::uninitialized_construct_using_allocator(p, a, 9);
    if (r != p || p->v != 9 || p->alloc_id != 42) return false;
    std::destroy_at(p);
    al.deallocate(p, 1);
    std::allocator<std::pair<Trailing, Plain>> pa;
    auto* pp = pa.allocate(1);
    std::uninitialized_construct_using_allocator(pp, a, 1, 2);
    if (pp->first.alloc_id != 42 || pp->second.v != 2) return false;
    std::destroy_at(pp);
    pa.deallocate(pp, 1);
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
