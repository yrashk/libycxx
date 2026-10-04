// [func.bind.bind]/4: for the second overload (bind<R>), g(u1, ..., uM) is
// expression-equivalent to INVOKE<R>(static_cast<Vfd>(vfd), static_cast<V1>(v1), ...).
// [func.require]/2: INVOKE<R>(f, t...) is static_cast<void>(INVOKE(f, t...)) if R is cv void,
// otherwise INVOKE(f, t...) implicitly converted to R.
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

using namespace std::placeholders;

constexpr int sub(int a, int b) { return a - b; }
constexpr int& ref(int& r) { return r; }
struct Wrapped {
  int v;
  constexpr Wrapped(int x) : v(x) {}  // implicit
};
struct S {
  int v;
  constexpr int get() const { return v; }
};
struct Target {
  constexpr int operator()() & { return 1; }
  constexpr int operator()() const& { return 2; }
};

constexpr bool test() {
  // conversion of the result
  auto d = std::bind<double>(sub, _1, 2);
  if (d(7) != 5.0) return false;
  if (std::bind<Wrapped>(sub, 10, 3)().v != 7) return false;
  if (std::bind<long long>(sub, _2, _1)(1, 3) != 2LL) return false;
  // a reference result type refers to the original object
  int x = 1;
  auto r = std::bind<int&>(ref, std::ref(x));
  r() = 5;
  if (x != 5) return false;
  auto cr = std::bind<const int&>(ref, _1);
  if (&cr(x) != &x) return false;
  // member pointers
  S s{3};
  if (std::bind<long>(&S::get, _1)(s) != 3L) return false;
  if (std::bind<long>(&S::v, &s)() != 3L) return false;
  // the target is called as cv FD&
  auto t = std::bind<long>(Target{});
  if (t() != 1L || std::as_const(t)() != 2L || std::move(t)() != 1L) return false;
  return true;
}
static_assert(test());

static_assert(std::is_same_v<decltype(std::bind<double>(sub, 1, 2)()), double>);
static_assert(std::is_same_v<decltype(std::bind<Wrapped>(sub, 1, 2)()), Wrapped>);
static_assert(std::is_same_v<decltype(std::bind<int&>(ref, _1)(std::declval<int&>())), int&>);
static_assert(std::is_same_v<decltype(std::bind<const void>(sub, 1, 2)()), void>);
static_assert(std::is_bind_expression_v<decltype(std::bind<void>(sub, 1, 2))>);

int main() {
  CHECK(test());
  int calls = 0;
  auto f = [&](int k) {
    calls += k;
    return calls;
  };
  std::bind<void>(f, _1)(2);
  std::bind<const volatile void>(f, 3)();
  CHECK(calls == 5);
  return 0;
}
