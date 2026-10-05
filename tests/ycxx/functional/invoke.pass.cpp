// [func.invoke]/1: invoke(f, args...) "Constraints: is_invocable_v<F, Args...> is true."
// "Returns: INVOKE(std::forward<F>(f), std::forward<Args>(args)...)". noexcept(
// is_nothrow_invocable_v<F, Args...>), constexpr. [func.require]/1: INVOKE for pointers to
// member functions/data with an object, a reference_wrapper or a pointer.
// COUNTERPART: libcxx:utilities/function.objects/func.invoke/invoke(_constexpr)?.pass.cpp
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct S {
  int v;
  constexpr int get() const { return v; }
  constexpr int add(int x) noexcept { return v + x; }
  constexpr int lref() & { return 1; }
  constexpr int rref() && { return 2; }
};
struct D : S {};
constexpr int twice(int x) noexcept { return 2 * x; }
int may_throw(int x) { return x; }

template <class... A>
concept can_invoke = requires(A&&... a) { std::invoke(std::forward<A>(a)...); };

static_assert(std::is_same_v<decltype(std::invoke(&S::v, std::declval<S&>())), int&>);
static_assert(std::is_same_v<decltype(std::invoke(&S::v, std::declval<S&&>())), int&&>);
static_assert(std::is_same_v<decltype(std::invoke(&S::v, std::declval<const S*>())), const int&>);
static_assert(noexcept(std::invoke(twice, 1)));
static_assert(!noexcept(std::invoke(may_throw, 1)));
static_assert(noexcept(std::invoke(&S::add, std::declval<S&>(), 1)));
static_assert(!can_invoke<int (*)(int), int, int>);
static_assert(!can_invoke<int S::*, int>);
static_assert(!can_invoke<decltype(&S::rref), S&>);
static_assert(can_invoke<decltype(&S::rref), S&&>);

constexpr bool test() {
  S s{5};
  D d;
  d.v = 6;
  const S* p = &s;
  if (std::invoke(twice, 3) != 6) return false;
  if (std::invoke(&S::get, s) != 5 || std::invoke(&S::get, p) != 5 || std::invoke(&S::get, d) != 6) return false;
  if (std::invoke(&S::get, std::ref(s)) != 5 || std::invoke(&S::get, std::cref(d)) != 6) return false;
  if (std::invoke(&S::add, s, 2) != 7) return false;
  if (std::invoke(&S::v, s) != 5 || std::invoke(&S::v, &d) != 6 || std::invoke(&S::v, std::ref(s)) != 5) return false;
  std::invoke(&S::v, s) = 9;
  if (s.v != 9) return false;
  if (std::invoke(&S::lref, s) != 1 || std::invoke(&S::rref, std::move(s)) != 2) return false;
  auto lam = [](int a, int b) { return a - b; };
  if (std::invoke(lam, 5, 3) != 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  CHECK(std::invoke(may_throw, 4) == 4);
  return 0;
}
