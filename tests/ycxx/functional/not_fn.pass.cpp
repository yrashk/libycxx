// [func.not.fn]/1-5: not_fn(f) returns "A perfect forwarding call wrapper g with call pattern
// !invoke(fd, call_args...)". /6-8 (C++26, P2714): template<auto f> constexpr unspecified
// not_fn() noexcept; whose call pattern is !invoke(f, call_args...).
// [func.require]/4: a perfect forwarding call wrapper delivers its state entity as cv T& when
// called as an lvalue and cv T&& otherwise.
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Cat {
  constexpr int operator()() & { return 1; }
  constexpr int operator()() const& { return 0; }
  constexpr int operator()() && { return 0; }
  constexpr int operator()() const&& { return 1; }
};
constexpr bool is_even(int x) { return x % 2 == 0; }
struct S {
  int v;
  constexpr bool odd() const { return v % 2; }
};

constexpr bool test() {
  auto ne = std::not_fn(is_even);
  if (ne(2) || !ne(3)) return false;
  auto nl = std::not_fn([](int a, int b) { return a < b; });
  if (nl(1, 2) || !nl(2, 1)) return false;
  // member pointers go through invoke
  auto nodd = std::not_fn(&S::odd);
  if (!nodd(S{2}) || nodd(S{3})) return false;
  auto nv = std::not_fn(&S::v);
  if (!nv(S{0}) || nv(S{1})) return false;
  // value category and constness of the wrapper are forwarded to the target
  auto nc = std::not_fn(Cat{});
  const auto& cnc = nc;
  if (nc() != false || cnc() != true || std::move(nc)() != true || std::move(cnc)() != false) return false;
  // C++26 constant-function form
  auto ne2 = std::not_fn<is_even>();
  if (ne2(4) || !ne2(5)) return false;
  auto nodd2 = std::not_fn<&S::odd>();
  if (nodd2(S{1})) return false;
  return true;
}
static_assert(test());

static_assert(noexcept(std::not_fn<is_even>()));
static_assert(std::is_empty_v<decltype(std::not_fn<is_even>())>);
// result is copyable/movable when the target is
static_assert(std::is_copy_constructible_v<decltype(std::not_fn(is_even))>);
static_assert(std::is_same_v<decltype(std::not_fn(is_even)(1)), bool>);

int main() {
  CHECK(test());
  return 0;
}
