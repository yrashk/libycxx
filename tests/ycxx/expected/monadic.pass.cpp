// [expected.object.monadic]: and_then (U = remove_cvref_t<invoke_result>; error propagated as
// U(unexpect, error())), or_else (G(in_place, val) when engaged), transform (U may be void,
// giving expected<void, E>; non-movable U direct-initialized), transform_error. Value
// categories of val/error are forwarded per overload.
#include <expected>
#include <type_traits>
#include <utility>
#include "check.hpp"

using E = std::expected<int, long>;

struct Cat {
  constexpr int operator()(int&) const { return 1; }
  constexpr int operator()(const int&) const { return 2; }
  constexpr int operator()(int&&) const { return 3; }
  constexpr int operator()(const int&&) const { return 4; }
  constexpr int operator()(long&) const { return 11; }
  constexpr int operator()(const long&) const { return 12; }
  constexpr int operator()(long&&) const { return 13; }
  constexpr int operator()(const long&&) const { return 14; }
};
struct CatE {
  template <class T> constexpr E operator()(T&& t) const { return E(Cat{}(std::forward<T>(t))); }
};
struct CatErr {
  template <class T> constexpr E operator()(T&& t) const { return E(std::unexpect, Cat{}(std::forward<T>(t))); }
};
struct NonMovable {
  int v;
  constexpr NonMovable(int x) : v(x) {}
  NonMovable(NonMovable&&) = delete;
};

constexpr bool test() {
  E v(5), e(std::unexpect, 7L);
  const E& cv = v;
  const E& ce = e;
  // and_then
  if (*v.and_then(CatE{}) != 1 || *cv.and_then(CatE{}) != 2) return false;
  if (*std::move(v).and_then(CatE{}) != 3 || *std::move(cv).and_then(CatE{}) != 4) return false;
  auto ae = e.and_then(CatE{});
  if (ae.has_value() || ae.error() != 7) return false;
  auto to_double = [](int x) { return std::expected<double, long>(x * 0.5); };
  static_assert(std::is_same_v<decltype(v.and_then(to_double)), std::expected<double, long>>);
  if (*v.and_then(to_double) != 2.5) return false;
  if (e.and_then(to_double).error() != 7) return false;
  // or_else
  if (e.or_else(CatErr{}).error() != 11 || ce.or_else(CatErr{}).error() != 12) return false;
  if (std::move(e).or_else(CatErr{}).error() != 13 || std::move(ce).or_else(CatErr{}).error() != 14) return false;
  auto ov = v.or_else([](long) { return std::expected<int, char>(std::unexpect, 'x'); });
  static_assert(std::is_same_v<decltype(ov), std::expected<int, char>>);
  if (!ov || *ov != 5) return false;
  // transform
  if (*v.transform(Cat{}) != 1 || *cv.transform(Cat{}) != 2) return false;
  if (*std::move(v).transform(Cat{}) != 3 || *std::move(cv).transform(Cat{}) != 4) return false;
  if (e.transform(Cat{}).error() != 7) return false;
  static_assert(std::is_same_v<decltype(v.transform([](int) { return 1.0; })), std::expected<double, long>>);
  static_assert(std::is_same_v<decltype(v.transform([](int) -> const char { return 'a'; })), std::expected<char, long>>);
  int calls = 0;
  auto tv = v.transform([&](int) { ++calls; });
  static_assert(std::is_same_v<decltype(tv), std::expected<void, long>>);
  if (!tv.has_value() || calls != 1) return false;
  auto tve = e.transform([&](int) { ++calls; });
  if (tve.has_value() || tve.error() != 7 || calls != 1) return false;
  auto nm = v.transform([](int x) { return NonMovable(x); });
  static_assert(std::is_same_v<decltype(nm), std::expected<NonMovable, long>>);
  if (nm->v != 5) return false;
  // transform_error
  if (e.transform_error(Cat{}).error() != 11 || ce.transform_error(Cat{}).error() != 12) return false;
  if (std::move(e).transform_error(Cat{}).error() != 13 || std::move(ce).transform_error(Cat{}).error() != 14) return false;
  auto te = v.transform_error([](long) { return 'c'; });
  static_assert(std::is_same_v<decltype(te), std::expected<int, char>>);
  if (!te || *te != 5) return false;
  auto nme = e.transform_error([](long x) { return NonMovable(static_cast<int>(x)); });
  static_assert(std::is_same_v<decltype(nme), std::expected<int, NonMovable>>);
  if (nme.error().v != 7) return false;
  return true;
}
static_assert(test());

// Constraints: and_then/transform need E constructible from error(); or_else/transform_error
// need T constructible from val.
struct MoveOnly {
  int v;
  constexpr MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&&) = default;
};
template <class X, class F> concept can_and_then = requires(F f) { std::declval<X>().and_then(f); };
template <class X, class F> concept can_or_else = requires(F f) { std::declval<X>().or_else(f); };
using ME = std::expected<int, MoveOnly>;
using MV = std::expected<MoveOnly, int>;
static_assert(!can_and_then<ME&, ME (*)(int)>);         // E not copy-constructible from error() lvalue
static_assert(can_and_then<ME&&, ME (*)(int)>);
static_assert(!can_or_else<MV&, MV (*)(int)>);           // T not constructible from val lvalue
static_assert(can_or_else<MV&&, MV (*)(int)>);

int main() {
  CHECK(test());
  return 0;
}
