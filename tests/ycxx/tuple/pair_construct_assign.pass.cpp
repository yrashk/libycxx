// [tuple.cnstr]/24-27: tuple constructors from pair<U1, U2>&, const pair&, pair&&,
// const pair&& (FWD(u) preserves the value category; explicit iff an element is not
// convertible). [tuple.cnstr]/20-23: the non-const lvalue tuple<UTypes...>& overload.
// [tuple.assign]/27-38: assignment from const pair& and pair&&.
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Exp { int v; constexpr explicit Exp(int x) : v(x) {} };
struct MoveOnly {
  int v;
  constexpr MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) : v(o.v) { o.v = -1; }
  MoveOnly(const MoveOnly&) = delete;
  constexpr MoveOnly& operator=(MoveOnly&& o) { v = o.v; o.v = -1; return *this; }
};
// Distinguishes the value category it was constructed from.
struct Cat {
  int kind;
  constexpr Cat(int&) : kind(1) {}
  constexpr Cat(const int&) : kind(2) {}
  constexpr Cat(int&&) : kind(3) {}
  constexpr Cat(const int&&) : kind(4) {}
};

// Explicitness.
static_assert(std::is_convertible_v<std::pair<int, long>, std::tuple<long, int>>);
static_assert(std::is_constructible_v<std::tuple<Exp, int>, std::pair<int, int>>);
static_assert(!std::is_convertible_v<std::pair<int, int>, std::tuple<Exp, int>>);
static_assert(!std::is_convertible_v<const std::pair<int, int>&, std::tuple<int, Exp>>);
// Constraints: exactly two elements, each constructible.
static_assert(!std::is_constructible_v<std::tuple<int>, std::pair<int, int>>);
static_assert(!std::is_constructible_v<std::tuple<int, int, int>, std::pair<int, int>>);
static_assert(!std::is_constructible_v<std::tuple<int*, int>, std::pair<int, int>>);
static_assert(!std::is_constructible_v<std::tuple<MoveOnly, int>, const std::pair<MoveOnly, int>&>);
static_assert(std::is_constructible_v<std::tuple<MoveOnly, int>, std::pair<MoveOnly, int>&&>);
// Non-const lvalue pair: binds non-const references.
static_assert(std::is_constructible_v<std::tuple<int&, int&>, std::pair<int, int>&>);
static_assert(!std::is_constructible_v<std::tuple<int&, int&>, const std::pair<int, int>&>);
static_assert(!std::is_constructible_v<std::tuple<int&, int&>, std::pair<int, int>&&>);
static_assert(std::is_constructible_v<std::tuple<const int&&, int>, const std::pair<int, int>&&>);
// Non-const lvalue tuple<UTypes...>& overload.
static_assert(std::is_constructible_v<std::tuple<int&, long&>, std::tuple<int, long>&>);
static_assert(!std::is_constructible_v<std::tuple<int&, long&>, const std::tuple<int, long>&>);
// Assignment constraints.
static_assert(std::is_assignable_v<std::tuple<long, int>&, const std::pair<int, short>&>);
static_assert(!std::is_assignable_v<std::tuple<long, int, int>&, const std::pair<int, short>&>);
static_assert(!std::is_assignable_v<std::tuple<MoveOnly, int>&, const std::pair<MoveOnly, int>&>);
static_assert(std::is_assignable_v<std::tuple<MoveOnly, int>&, std::pair<MoveOnly, int>&&>);
static_assert(!std::is_assignable_v<std::tuple<int*, int>&, std::pair<int, int>>);

constexpr bool test() {
  {
    std::pair<int, long> p(1, 2);
    std::tuple<long, int> t = p;                         // const pair& (implicit)
    if (std::get<0>(t) != 1 || std::get<1>(t) != 2) return false;
    std::tuple<Exp, int> e(p);                           // explicit
    if (std::get<0>(e).v != 1) return false;
  }
  {
    std::pair<int, int> p(3, 4);
    std::tuple<int&, int&> r(p);                         // pair& overload
    std::get<0>(r) = 30;
    if (p.first != 30) return false;
    if (&std::get<1>(r) != &p.second) return false;
  }
  {
    std::pair<int, int> p(5, 6);
    std::tuple<Cat, Cat> a(p);
    std::tuple<Cat, Cat> b(std::as_const(p));
    std::tuple<Cat, Cat> c(std::move(p));
    std::tuple<Cat, Cat> d(std::move(std::as_const(p)));
    if (std::get<0>(a).kind != 1 || std::get<1>(b).kind != 2) return false;
    if (std::get<0>(c).kind != 3 || std::get<1>(d).kind != 4) return false;
  }
  {
    const std::pair<int, int> cp(7, 8);
    std::tuple<const int&&, int> t(std::move(cp));
    if (&std::get<0>(t) != &cp.first) return false;
  }
  {
    std::pair<MoveOnly, int> p(MoveOnly(9), 10);
    std::tuple<MoveOnly, int> t(std::move(p));
    if (std::get<0>(t).v != 9 || p.first.v != -1) return false;
  }
  {
    std::tuple<int, long> src(11, 12);
    std::tuple<int&, long&> r(src);                      // tuple<UTypes...>& overload
    if (&std::get<1>(r) != &std::get<1>(src)) return false;
  }
  {
    std::tuple<long, int> t(0, 0);
    const std::pair<int, short> p(13, 14);
    auto& ret = (t = p);
    if (&ret != &t || std::get<0>(t) != 13 || std::get<1>(t) != 14) return false;
    t = std::make_pair(15, 16);
    if (std::get<0>(t) != 15 || std::get<1>(t) != 16) return false;
    static_assert(std::is_same_v<decltype(t = p), std::tuple<long, int>&>);
  }
  {
    std::tuple<MoveOnly, int> t(MoveOnly(0), 0);
    std::pair<MoveOnly, int> p(MoveOnly(17), 18);
    t = std::move(p);
    if (std::get<0>(t).v != 17 || p.first.v != -1 || std::get<1>(t) != 18) return false;
  }
  {
    // CTAD from pair.
    std::pair<int, double> p(1, 2.0);
    std::tuple t(p);
    static_assert(std::is_same_v<decltype(t), std::tuple<int, double>>);
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
