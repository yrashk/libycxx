// [func.bind.partial]/9: template<auto f, class... Args> bind_front(args...) / bind_back
// return "A perfect forwarding call wrapper g whose target object is a copy of cw<f>, and whose
// call pattern is invoke(f, bound_args..., call_args...)" / "invoke(f, call_args...,
// bound_args...)". [func.not.fn]/8: not_fn<f>() likewise with !invoke(f, call_args...).
// In the call pattern f names the template parameter object itself (a const lvalue), so the
// target is always invoked as a const lvalue, whatever the wrapper's value category; bound
// arguments are still forwarded per [func.require]/4.
// COUNTERPART: libcxx:utilities/function.objects/func.bind.partial/bind_front.nttp.pass.cpp
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Q {
  constexpr int operator()() & { return 1; }
  constexpr int operator()() const& { return 2; }
  constexpr int operator()() && { return 3; }
  constexpr int operator()() const&& { return 4; }
};
struct QB {
  constexpr bool operator()() & { return false; }
  constexpr bool operator()() const& { return true; }
  constexpr bool operator()() && { return false; }
  constexpr bool operator()() const&& { return false; }
};
struct Arg {
  constexpr int operator()(int&) const { return 1; }
  constexpr int operator()(const int&) const { return 2; }
  constexpr int operator()(int&&) const { return 3; }
  constexpr int operator()(const int&&) const { return 4; }
};
struct S {
  int v;
};
constexpr int sub(int a, int b) { return a - b; }

constexpr bool test() {
  auto g = std::bind_front<Q{}>();
  const auto& cg = g;
  if (g() != 2 || cg() != 2 || std::move(g)() != 2 || std::move(cg)() != 2) return false;
  auto h = std::bind_back<Q{}>();
  if (h() != 2 || std::move(h)() != 2) return false;
  auto n = std::not_fn<QB{}>();
  if (n() || std::move(n)()) return false;  // const& overload gives true; negated
  // bound arguments follow the wrapper
  auto a = std::bind_front<Arg{}>(0);
  const auto& ca = a;
  if (a() != 1 || ca() != 2 || std::move(a)() != 3 || std::move(ca)() != 4) return false;
  auto ab = std::bind_back<Arg{}>(0);
  if (ab() != 1 || std::move(ab)() != 3) return false;
  // a closure type as the constant target
  if (std::bind_front<[](int p, int q) { return p * q; }>(6)(7) != 42) return false;
  if (std::bind_back<sub>(1)(10) != 9) return false;
  // pointer to data member, call argument as object
  S s{5};
  std::bind_back<&S::v>()(s) = 6;
  if (s.v != 6) return false;
  return true;
}
static_assert(test());

static_assert(std::is_empty_v<decltype(std::bind_back<sub>())>);
static_assert(std::is_empty_v<decltype(std::not_fn<sub>())>);
static_assert(std::is_same_v<decltype(std::bind_front<sub>(1)), decltype(std::bind_front<sub>(2))>);
static_assert(std::is_same_v<decltype(std::bind_back<sub>(1)), decltype(std::bind_back<sub>(std::declval<const int&>()))>);
static_assert(std::is_same_v<decltype(std::bind_back<&S::v>()(std::declval<S&>())), int&>);
static_assert(std::is_same_v<decltype(std::bind_back<&S::v>()(std::declval<S>())), int&&>);
static_assert(!std::is_invocable_v<decltype(std::bind_front<sub>(1))>);
static_assert(!std::is_invocable_v<decltype(std::bind_front<sub>(1)), int*>);
static_assert(std::is_invocable_r_v<int, decltype(std::bind_front<sub>(1)), int>);

int main() {
  CHECK(test());
  return 0;
}
