// [variant.visit]: std::visit with one and several variants (value category forwarded through
// GET<m>(std::forward<V>(vars))...), return type decltype(e(m)); visit<R> converts the result
// via INVOKE<R> (R = void discards); as-variant lets classes derived from variant be visited;
// member visit(this Self&&, Visitor&&) forwards self's value category and constness.
#include <variant>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Cat {
  constexpr int operator()(int&) const { return 1; }
  constexpr int operator()(const int&) const { return 2; }
  constexpr int operator()(int&&) const { return 3; }
  constexpr int operator()(const int&&) const { return 4; }
  constexpr int operator()(double&) const { return 11; }
  constexpr int operator()(const double&) const { return 12; }
  constexpr int operator()(double&&) const { return 13; }
  constexpr int operator()(const double&&) const { return 14; }
};

struct Derived : std::variant<int, double> {
  using std::variant<int, double>::variant;
};

struct Sum {
  constexpr long operator()(int a, int b) const { return a + b; }
  constexpr long operator()(int a, double b) const { return a + static_cast<long>(b) * 10; }
  constexpr long operator()(double a, int b) const { return static_cast<long>(a) * 100 + b; }
  constexpr long operator()(double a, double b) const { return static_cast<long>(a + b) * 1000; }
};

struct RefRet {
  int x = 5;
  constexpr int& operator()(auto) { return x; }
};

constexpr bool test() {
  using V = std::variant<int, double>;
  V v(1);
  const V cv(2.0);
  if (std::visit(Cat{}, v) != 1) return false;
  if (std::visit(Cat{}, std::as_const(v)) != 2) return false;
  if (std::visit(Cat{}, std::move(v)) != 3) return false;
  if (std::visit(Cat{}, std::move(std::as_const(v))) != 4) return false;
  if (std::visit(Cat{}, cv) != 12) return false;
  if (std::visit(Cat{}, std::move(cv)) != 14) return false;

  // member visit
  if (v.visit(Cat{}) != 1) return false;
  if (std::as_const(v).visit(Cat{}) != 2) return false;
  if (std::move(v).visit(Cat{}) != 3) return false;
  if (cv.visit(Cat{}) != 12) return false;

  // multiple variants
  V a(1), b(2.0);
  if (std::visit(Sum{}, a, a) != 2) return false;
  if (std::visit(Sum{}, a, b) != 21) return false;
  if (std::visit(Sum{}, b, a) != 201) return false;
  if (std::visit(Sum{}, b, b) != 4000) return false;
  // zero variants
  if (std::visit([] { return 7; }) != 7) return false;

  // visit<R>
  static_assert(std::is_same_v<decltype(std::visit<double>(Sum{}, a, b)), double>);
  if (std::visit<double>(Sum{}, a, b) != 21.0) return false;
  static_assert(std::is_same_v<decltype(std::visit<void>(Sum{}, a, b)), void>);
  static_assert(std::is_same_v<decltype(v.visit<long long>(Cat{})), long long>);
  if (v.visit<long long>(Cat{}) != 1) return false;

  // reference results are preserved
  RefRet rr;
  static_assert(std::is_same_v<decltype(std::visit(rr, v)), int&>);
  if (&std::visit(rr, v) != &rr.x) return false;

  // derived from variant
  Derived d(2.5);
  if (std::visit(Cat{}, d) != 11) return false;
  if (std::visit(Cat{}, std::move(d)) != 13) return false;
  if (d.visit(Cat{}) != 11) return false;
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
  // visit<void> discards a non-void result
  std::variant<int, double> v(1);
  int calls = 0;
  std::visit<void>([&](auto x) { ++calls; return x; }, v);
  v.visit<void>([&](auto x) { ++calls; return x; });
  CHECK(calls == 2);
  return 0;
}
