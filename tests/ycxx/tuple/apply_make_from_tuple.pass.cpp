// [tuple.apply]/1: apply(f, t) is INVOKE(std::forward<F>(f), get<I>(std::forward<Tuple>(t))...)
// with return type apply_result_t<F, Tuple> and noexcept(is_nothrow_applicable_v<F, Tuple>).
// [tuple.apply]/2-3: make_from_tuple<T>(t) is T(get<I>(std::forward<Tuple>(t))...).
// COUNTERPART: libstdcxx:20_util/tuple/dr3528.cc
// COUNTERPART: libcxx:utilities/tuple/tuple.tuple/tuple.apply/make_from_tuple.pass.cpp
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct S {
  int m = 0;
  constexpr int add(int a, int b) const { return m + a + b; }
};
struct Cat {
  constexpr int operator()(int&) const { return 1; }
  constexpr int operator()(const int&) const { return 2; }
  constexpr int operator()(int&&) const { return 3; }
  constexpr int operator()(const int&&) const { return 4; }
};
struct RefQualified {
  constexpr int operator()() & { return 1; }
  constexpr int operator()() && { return 2; }
};
struct Point {
  int x, y;
  constexpr Point(int a, int b) : x(a), y(b) {}
};
struct ExplicitOne { int v; constexpr explicit ExplicitOne(int a) : v(a) {} };
struct DefaultOnly { int v = 77; };
struct MoveOnly {
  int v;
  constexpr MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) : v(o.v) { o.v = -1; }
  MoveOnly(const MoveOnly&) = delete;
};
struct TakesMoveOnly { int v; constexpr TakesMoveOnly(MoveOnly m) : v(m.v) {} };
constexpr int nothrow_fn(int a) noexcept { return a; }
constexpr int throwing_fn(int a) { return a; }

constexpr int& first_ref(int& a, int&) { return a; }

static_assert(noexcept(std::apply(nothrow_fn, std::tuple<int>(1))));
static_assert(!noexcept(std::apply(throwing_fn, std::tuple<int>(1))));
static_assert(std::is_same_v<decltype(std::apply(first_ref, std::declval<std::tuple<int, int>&>())), int&>);
static_assert(std::is_same_v<decltype(std::apply(&S::m, std::declval<std::tuple<S&>>())), int&>);
static_assert(std::is_same_v<decltype(std::apply(&S::m, std::declval<std::tuple<S>>())), int&&>);
static_assert(std::is_same_v<decltype(std::make_from_tuple<Point>(std::tuple<int, int>(1, 2))), Point>);

constexpr bool test_apply() {
  if (std::apply([](int a, long b) { return a * 10 + b; }, std::tuple<int, long>(1, 2)) != 12) return false;
  if (std::apply([](int a, long b) { return a - b; }, std::pair<int, long>(5, 7)) != -2) return false;
  if (std::apply([] { return 42; }, std::tuple<>{}) != 42) return false;
  // Value category of the elements follows the tuple.
  std::tuple<int> t(0);
  if (std::apply(Cat{}, t) != 1) return false;
  if (std::apply(Cat{}, std::as_const(t)) != 2) return false;
  if (std::apply(Cat{}, std::move(t)) != 3) return false;
  if (std::apply(Cat{}, std::move(std::as_const(t))) != 4) return false;
  // Value category of the callable is forwarded.
  RefQualified rq;
  if (std::apply(rq, std::tuple<>{}) != 1 || std::apply(RefQualified{}, std::tuple<>{}) != 2) return false;
  // Pointers to members.
  S s{5};
  if (std::apply(&S::add, std::tuple<S&, int, int>(s, 1, 2)) != 8) return false;
  if (std::apply(&S::add, std::tuple<const S*, int, int>(&s, 3, 4)) != 12) return false;
  std::apply(&S::m, std::tuple<S&>(s)) = 9;
  if (s.m != 9) return false;
  // References in the tuple refer to the original objects.
  int a = 1, b = 2;
  std::apply(first_ref, std::tie(a, b)) = 100;
  if (a != 100) return false;
  // Move-only elements are moved out of an rvalue tuple.
  std::tuple<MoveOnly> mt(MoveOnly(6));
  int got = std::apply([](MoveOnly m) { return m.v; }, std::move(mt));
  if (got != 6 || std::get<0>(mt).v != -1) return false;
  return true;
}

constexpr bool test_make_from_tuple() {
  Point p = std::make_from_tuple<Point>(std::tuple<int, int>(3, 4));
  if (p.x != 3 || p.y != 4) return false;
  Point q = std::make_from_tuple<Point>(std::pair<short, long>(5, 6));
  if (q.x != 5 || q.y != 6) return false;
  ExplicitOne e = std::make_from_tuple<ExplicitOne>(std::tuple<int>(7));   // direct-initialization
  if (e.v != 7) return false;
  DefaultOnly d = std::make_from_tuple<DefaultOnly>(std::tuple<>{});
  if (d.v != 77) return false;
  std::tuple<MoveOnly> mt(MoveOnly(8));
  TakesMoveOnly tm = std::make_from_tuple<TakesMoveOnly>(std::move(mt));
  if (tm.v != 8 || std::get<0>(mt).v != -1) return false;
  int x = 9;
  int& r = std::make_from_tuple<int&>(std::tuple<int&>(x));                // binds directly: OK
  if (&r != &x) return false;
  const int& cr = std::make_from_tuple<const int&>(std::tuple<int&>(x));
  if (&cr != &x) return false;
  return true;
}

static_assert(test_apply());
static_assert(test_make_from_tuple());

int main() {
  CHECK(test_apply());
  CHECK(test_make_from_tuple());
  return 0;
}
