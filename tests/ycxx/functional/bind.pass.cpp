// [func.bind.bind]/4: g(u1, ..., uM) is expression-equivalent to INVOKE(static_cast<Vfd>(vfd),
// static_cast<V1>(v1), ..., static_cast<VN>(vN)) (INVOKE<R> for bind<R>), where /7: a
// reference_wrapper<T> bound argument yields tdi.get() (T&); a bind expression yields
// static_cast<cv TDi&>(tdi)(std::forward<Uj>(uj)...); placeholder _j yields
// std::forward<Uj>(uj); anything else yields tdi as cv TDi&. /8: vfd is fd, of type cv FD&.
// bind is constexpr.
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

using namespace std::placeholders;

constexpr int sub(int a, int b) { return a - b; }
constexpr int add3(int a, int b, int c) { return a + b + c; }
constexpr int twice(int a) { return 2 * a; }
constexpr int inc(int& r) { return ++r; }
constexpr int take(int&& r) { return r * 10; }
struct S {
  int v;
  constexpr int get() const { return v; }
  constexpr int plus(int k) const { return v + k; }
};
struct Probe {  // reports how the bound argument arrives
  constexpr int operator()(int&) const { return 1; }
  constexpr int operator()(const int&) const { return 2; }
  constexpr int operator()(int&&) const { return 3; }
};

constexpr bool test() {
  if (std::bind(sub, 10, 3)() != 7) return false;
  if (std::bind(sub, _1, 3)(10) != 7) return false;
  if (std::bind(sub, _2, _1)(3, 10) != 7) return false;   // reordering
  if (std::bind(sub, _1, _1)(5) != 0) return false;       // repetition
  if (std::bind(twice, _2)(100, 4) != 8) return false;    // unused arguments are ignored
  if (std::bind(sub, 1, 2)(7, 8, 9) != -1) return false;  // extra arguments are ignored
  if (std::bind(add3, _3, _1, _2)(1, 20, 300) != 321) return false;
  // nested bind expressions are evaluated with the same arguments
  if (std::bind(sub, std::bind(twice, _1), _2)(5, 1) != 9) return false;
  if (std::bind(twice, std::bind(twice, std::bind(twice, _1)))(1) != 8) return false;
  // member pointers
  S s{4};
  if (std::bind(&S::get, _1)(s) != 4) return false;
  if (std::bind(&S::plus, &s, _1)(3) != 7) return false;
  if (std::bind(&S::v, _1)(s) != 4) return false;
  // bind<R>
  static_assert(std::is_same_v<decltype(std::bind<long>(sub, 1, 2)()), long>);
  if (std::bind<long>(sub, 1, 2)() != -1L) return false;
  // bound values are copies, passed as lvalues
  int x = 1;
  auto byval = std::bind(inc, x);
  if (byval() != 2 || byval() != 3 || x != 1) return false;
  // reference_wrapper binds by reference
  auto byref = std::bind(inc, std::ref(x));
  byref();
  if (x != 2) return false;
  // placeholders forward the call argument's value category
  if (std::bind(inc, _1)(x) != 3 || x != 3) return false;
  if (std::bind(take, _1)(4) != 40) return false;
  if (std::bind(Probe{}, _1)(x) != 1) return false;
  if (std::bind(Probe{}, _1)(std::move(x)) != 3) return false;
  // a non-placeholder bound argument is cv TDi&: lvalue, const if the wrapper is const
  auto pb = std::bind(Probe{}, 0);
  const auto& cpb = pb;
  if (pb() != 1 || cpb() != 2 || std::move(pb)() != 1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // bind<void> discards the result
  int calls = 0;
  auto f = [&] { return ++calls; };
  std::bind<void>(f)();
  CHECK(calls == 1);
  static_assert(std::is_void_v<decltype(std::bind<void>(f)())>);
  // copies of a bind expression are independent (Cpp17CopyConstructible when all parts are)
  auto counter = std::bind([](int& n) { return ++n; }, 0);
  auto copy = counter;
  counter();
  counter();
  CHECK(copy() == 1);
  return 0;
}
