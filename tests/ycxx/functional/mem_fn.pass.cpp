// [func.memfn]: template<class R, class T> constexpr unspecified mem_fn(R T::* pm) noexcept;
// "Returns: A simple call wrapper fn with call pattern invoke(pmd, call_args...), where pmd
// is the target object of fn of type R T::* direct-non-list-initialized with pm".
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct S {
  int v;
  constexpr int get() const { return v; }
  constexpr int add(int a, int b) noexcept { return v + a + b; }
  constexpr int rv() && { return 7; }
};
struct D : S {};

static_assert(noexcept(std::mem_fn(&S::get)));
static_assert(std::is_same_v<decltype(std::mem_fn(&S::v)(std::declval<S&>())), int&>);
static_assert(std::is_same_v<decltype(std::mem_fn(&S::v)(std::declval<const S*>())), const int&>);
static_assert(std::is_same_v<decltype(std::mem_fn(&S::v)(std::declval<S>())), int&&>);
static_assert(std::is_nothrow_invocable_v<decltype(std::mem_fn(&S::add)), S&, int, int>);
static_assert(std::is_copy_constructible_v<decltype(std::mem_fn(&S::get))>);
static_assert(!std::is_invocable_v<decltype(std::mem_fn(&S::rv)), S&>);

constexpr bool test() {
  S s{3};
  D d;
  d.v = 4;
  auto g = std::mem_fn(&S::get);
  if (g(s) != 3 || g(&s) != 3 || g(d) != 4 || g(std::ref(s)) != 3) return false;
  if (std::mem_fn(&S::add)(s, 1, 2) != 6) return false;
  if (std::mem_fn(&S::rv)(S{0}) != 7) return false;
  std::mem_fn(&S::v)(s) = 9;
  if (s.v != 9) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
