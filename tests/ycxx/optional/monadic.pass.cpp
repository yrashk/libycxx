// [optional.monadic]: and_then (result type remove_cvref_t<invoke_result>), transform
// (U = remove_cv_t<invoke_result_t<F, decltype((val))>>; U may be an lvalue reference,
// giving optional<T&>; non-movable U is direct-initialized), or_else (returns optional;
// && overload moves). Value category of val is forwarded per overload.
#include <optional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Cat {
  constexpr int operator()(int&) const { return 1; }
  constexpr int operator()(const int&) const { return 2; }
  constexpr int operator()(int&&) const { return 3; }
  constexpr int operator()(const int&&) const { return 4; }
};
struct CatOpt {
  constexpr std::optional<int> operator()(int&) const { return 1; }
  constexpr std::optional<int> operator()(const int&) const { return 2; }
  constexpr std::optional<int> operator()(int&&) const { return 3; }
  constexpr std::optional<int> operator()(const int&&) const { return 4; }
};
struct NonMovable {
  int v;
  constexpr NonMovable(int x) : v(x) {}
  NonMovable(NonMovable&&) = delete;
};

constexpr bool test() {
  std::optional<int> o(2), e;
  const std::optional<int>& co = o;
  // and_then
  if (o.and_then(CatOpt{}) != 1 || co.and_then(CatOpt{}) != 2) return false;
  if (std::move(o).and_then(CatOpt{}) != 3 || std::move(co).and_then(CatOpt{}) != 4) return false;
  if (e.and_then(CatOpt{}).has_value()) return false;
  auto half = [](int x) -> std::optional<long> { if (x % 2) return std::nullopt; return x / 2; };
  static_assert(std::is_same_v<decltype(o.and_then(half)), std::optional<long>>);
  if (o.and_then(half) != 1L) return false;
  if (std::optional<int>(3).and_then(half).has_value()) return false;
  // and_then with a function returning a reference to optional: result is remove_cvref_t
  std::optional<long> store(9);
  auto getref = [&](int) -> std::optional<long>& { return store; };
  static_assert(std::is_same_v<decltype(o.and_then(getref)), std::optional<long>>);
  if (o.and_then(getref) != 9L) return false;

  // transform
  if (o.transform(Cat{}) != 1 || co.transform(Cat{}) != 2) return false;
  if (std::move(o).transform(Cat{}) != 3 || std::move(co).transform(Cat{}) != 4) return false;
  if (e.transform(Cat{}).has_value()) return false;
  static_assert(std::is_same_v<decltype(o.transform([](int x) { return x * 1.5; })), std::optional<double>>);
  // cv-qualified prvalue result: U = remove_cv_t
  static_assert(std::is_same_v<decltype(o.transform([](int) -> const long { return 1; })), std::optional<long>>);
  // reference result -> optional<T&>
  int target = 0;
  auto ref = [&](int) -> int& { return target; };
  static_assert(std::is_same_v<decltype(o.transform(ref)), std::optional<int&>>);
  auto r = o.transform(ref);
  if (&*r != &target) return false;
  // non-movable result
  auto nm = o.transform([](int x) { return NonMovable(x); });
  static_assert(std::is_same_v<decltype(nm), std::optional<NonMovable>>);
  if (nm->v != 2) return false;

  // or_else
  int calls = 0;
  auto fallback = [&] { ++calls; return std::optional<int>(7); };
  if (o.or_else(fallback) != 2 || calls != 0) return false;
  if (e.or_else(fallback) != 7 || calls != 1) return false;
  if (std::move(e).or_else(fallback) != 7 || calls != 2) return false;
  static_assert(std::is_same_v<decltype(o.or_else(fallback)), std::optional<int>>);
  return true;
}
static_assert(test());

struct MoveOnly {
  int v;
  constexpr MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) : v(o.v) { o.v = -1; }
};
template <class O, class F> concept can_or_else = requires(O o, F f) { std::forward<O>(o).or_else(f); };
using MF = std::optional<MoveOnly> (*)();
static_assert(can_or_else<std::optional<MoveOnly>&&, MF>);
static_assert(!can_or_else<std::optional<MoveOnly>&, MF>);  // const& overload needs copy_constructible

constexpr bool test_move_only() {
  std::optional<MoveOnly> m(std::in_place, 5);
  auto r = std::move(m).or_else([] { return std::optional<MoveOnly>(1); });
  if (r->v != 5 || m->v != -1) return false;
  auto t = std::move(r).transform([](MoveOnly&& x) { return MoveOnly(std::move(x)); });
  if (t->v != 5 || r->v != -1) return false;
  return true;
}
static_assert(test_move_only());

int main() {
  CHECK(test());
  CHECK(test_move_only());
  return 0;
}
