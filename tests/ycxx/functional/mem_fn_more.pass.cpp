// [func.memfn]/1-2: mem_fn(pm) returns "A simple call wrapper fn with call pattern
// invoke(pmd, call_args...)". [func.require]/1: INVOKE(f, t1, t2, ..., tN) for member function
// pointers uses (t1.*f)(...), (t1.get().*f)(...) for a reference_wrapper, or ((*t1).*f)(...)
// otherwise (pointers and pointer-like types); likewise t1.*f, t1.get().*f, (*t1).*f for data
// members.
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct S {
  int v;
  constexpr int get() const { return v; }
  constexpr int& ref() & { return v; }
  constexpr int crv() const&& { return -v; }
  constexpr int take(int&& r) const { return v + r; }
  int va(int n, ...) const { return v + n; }
  int vol() volatile { return 1; }
};
struct D : S {};
struct Ptr {  // pointer-like: has operator*
  S* p;
  constexpr S& operator*() const { return *p; }
};

constexpr bool test() {
  S s{4};
  D d;
  d.v = 6;
  auto get = std::mem_fn(&S::get);
  if (get(Ptr{&s}) != 4 || get(&d) != 6 || get(std::cref(d)) != 6) return false;
  std::mem_fn(&S::ref)(s) = 5;
  if (s.v != 5) return false;
  if (std::mem_fn(&S::crv)(std::move(std::as_const(s))) != -5) return false;
  if (std::mem_fn(&S::crv)(S{2}) != -2) return false;
  if (std::mem_fn(&S::take)(s, 1) != 6) return false;
  std::mem_fn(&S::v)(Ptr{&s}) = 8;
  if (s.v != 8) return false;
  auto mf = std::mem_fn(&S::v);
  auto mf2 = mf;  // a simple call wrapper: copyable
  mf2 = mf;       // and copy-assignable
  if (mf2(s) != 8) return false;
  return true;
}
static_assert(test());

using G = decltype(std::mem_fn(&S::get));
static_assert(std::is_invocable_r_v<int, G, Ptr>);
static_assert(std::is_invocable_r_v<int, G, std::reference_wrapper<const S>>);
static_assert(!std::is_invocable_v<G, int>);
static_assert(!std::is_invocable_v<G>);
static_assert(!std::is_invocable_v<G, S&, int>);
using R = decltype(std::mem_fn(&S::ref));
static_assert(std::is_same_v<std::invoke_result_t<R, S&>, int&>);
static_assert(!std::is_invocable_v<R, S>);
static_assert(!std::is_invocable_v<R, const S&>);
static_assert(std::is_invocable_v<R, Ptr>);
static_assert(std::is_invocable_v<R, S*>);
using CR = decltype(std::mem_fn(&S::crv));
static_assert(std::is_invocable_v<CR, S>);
static_assert(std::is_invocable_v<CR, const S>);
static_assert(!std::is_invocable_v<CR, S&>);
using T = decltype(std::mem_fn(&S::take));
static_assert(std::is_invocable_v<T, S&, int>);
static_assert(!std::is_invocable_v<T, S&, int&>);  // the argument is forwarded as an lvalue
using M = decltype(std::mem_fn(&S::v));
static_assert(std::is_same_v<std::invoke_result_t<M, Ptr>, int&>);
static_assert(std::is_same_v<std::invoke_result_t<M, const S>, const int&&>);
static_assert(std::is_same_v<std::invoke_result_t<M, std::reference_wrapper<S>>, int&>);
static_assert(std::is_same_v<std::invoke_result_t<M, const D*>, const int&>);
static_assert(std::is_invocable_v<decltype(std::mem_fn(&S::vol)), volatile S&>);
static_assert(!std::is_invocable_v<decltype(std::mem_fn(&S::vol)), const S&>);

int main() {
  CHECK(test());
  S s{1};
  CHECK(std::mem_fn(&S::va)(s, 2, 3.0, "x") == 3);
  return 0;
}
