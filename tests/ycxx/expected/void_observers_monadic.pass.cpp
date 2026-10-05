// [expected.void.obs]: operator* (void, noexcept), value() (throws bad_expected_access<E>),
// error() value categories, has_error, error_or.
// [expected.void.monadic]: and_then / transform invoke F with no arguments; or_else and
// transform_error pass error(); or_else returns G() when engaged.
// [expected.void.swap]: swap in all state combinations; noexcept.
// REQUIRES: exceptions
#include <expected>
#include <type_traits>
#include <utility>
#include "check.hpp"

using V = std::expected<void, long>;
static_assert(std::is_same_v<decltype(*std::declval<V&>()), void>);
static_assert(noexcept(*std::declval<const V&>()));
static_assert(std::is_same_v<decltype(std::declval<V&>().value()), void>);
static_assert(std::is_same_v<decltype(std::declval<V&>().error()), long&>);
static_assert(std::is_same_v<decltype(std::declval<V&&>().error()), long&&>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().error()), const long&>);
static_assert(std::is_same_v<decltype(std::declval<const V&&>().error()), const long&&>);
static_assert(noexcept(std::declval<V&>().error()) && noexcept(std::declval<const V&>().has_error()));
static_assert(std::is_same_v<decltype(std::declval<V&>().error_or(1)), long>);
static_assert(std::is_nothrow_swappable_v<V>);

struct Cat {
  constexpr int operator()(long&) const { return 11; }
  constexpr int operator()(const long&) const { return 12; }
  constexpr int operator()(long&&) const { return 13; }
  constexpr int operator()(const long&&) const { return 14; }
};
struct CatErr {
  template <class T> constexpr V operator()(T&& t) const { return V(std::unexpect, Cat{}(std::forward<T>(t))); }
};

constexpr bool test() {
  V v, e(std::unexpect, 7L);
  const V& ce = e;
  *v;
  if (v.error_or(9) != 9 || e.error_or(9) != 7 || std::move(e).error_or({}) != 7) return false;
  // and_then
  int calls = 0;
  auto r = v.and_then([&] { ++calls; return std::expected<int, long>(5); });
  static_assert(std::is_same_v<decltype(r), std::expected<int, long>>);
  if (*r != 5 || calls != 1) return false;
  auto re = e.and_then([&] { ++calls; return std::expected<int, long>(5); });
  if (re || re.error() != 7 || calls != 1) return false;
  // or_else
  if (e.or_else(CatErr{}).error() != 11 || ce.or_else(CatErr{}).error() != 12) return false;
  if (std::move(e).or_else(CatErr{}).error() != 13 || std::move(ce).or_else(CatErr{}).error() != 14) return false;
  auto ov = v.or_else([](long) { return std::expected<void, int>(std::unexpect, 1); });
  static_assert(std::is_same_v<decltype(ov), std::expected<void, int>>);
  if (!ov) return false;
  // transform
  auto t = v.transform([] { return 2.5; });
  static_assert(std::is_same_v<decltype(t), std::expected<double, long>>);
  if (*t != 2.5) return false;
  auto tv = v.transform([&] { ++calls; });
  static_assert(std::is_same_v<decltype(tv), V>);
  if (!tv || calls != 2) return false;
  if (e.transform([] { return 1; }).error() != 7) return false;
  // transform_error
  if (e.transform_error(Cat{}).error() != 11 || ce.transform_error(Cat{}).error() != 12) return false;
  if (std::move(e).transform_error(Cat{}).error() != 13) return false;
  auto tev = v.transform_error([](long) { return 'c'; });
  static_assert(std::is_same_v<decltype(tev), std::expected<void, char>>);
  if (!tev) return false;
  // swap: value/value, error/error, value/error, error/value
  V a, b, x(std::unexpect, 1L), y(std::unexpect, 2L);
  a.swap(b);
  if (!a || !b) return false;
  x.swap(y);
  if (x.error() != 2 || y.error() != 1) return false;
  a.swap(x);
  if (a || a.error() != 2 || !x) return false;
  swap(a, x);
  if (!a || x || x.error() != 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  V e(std::unexpect, 3L);
  bool caught = false;
  try { e.value(); } catch (const std::bad_expected_access<long>& ex) { caught = ex.error() == 3; }
  CHECK(caught);
  caught = false;
  try { std::move(e).value(); } catch (const std::bad_expected_access<long>& ex) { caught = ex.error() == 3; }
  CHECK(caught);
  V v;
  v.value();  // no throw
  return 0;
}
