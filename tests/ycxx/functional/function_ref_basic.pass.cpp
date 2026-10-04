// [func.wrap.ref.ctor]/3-5: function_ref(F* f) binds f; calls are invoke_r<R>(f, call-args...).
// /6-8: function_ref(F&& f) binds addressof(f); calls are invoke_r<R>(static_cast<cv T&>(f),
// call-args...). [func.wrap.ref.inv]/1: operator() "Equivalent to: return
// thunk-ptr(bound-entity, std::forward<ArgTypes>(args)...);". [func.wrap.ref.class]/2: every
// specialization is trivially copyable and models copyable.
#include <functional>
#include <concepts>
#include <type_traits>
#include <utility>
#include "check.hpp"

int twice(int x) { return 2 * x; }
int sum_with(std::function_ref<int(int)> f, int n) {
  int s = 0;
  for (int i = 0; i < n; ++i) s += f(i);
  return s;
}
struct Both {
  int operator()() { return 1; }
  int operator()() const { return 2; }
};
struct Stateful {
  int n = 0;
  int operator()(int k) { return n += k; }
};

using FR = std::function_ref<int(int)>;
static_assert(std::is_trivially_copyable_v<FR>);
static_assert(std::is_trivially_copyable_v<std::function_ref<void() const noexcept>>);
static_assert(std::copyable<FR>);
static_assert(std::copyable<std::function_ref<int(int) const>>);
static_assert(!std::is_default_constructible_v<FR>);
static_assert(std::is_nothrow_copy_constructible_v<FR>);
static_assert(std::is_nothrow_copy_assignable_v<FR>);
static_assert(std::is_nothrow_constructible_v<FR, int (*)(int)>);
static_assert(std::is_nothrow_constructible_v<FR, Stateful&>);
static_assert(std::is_invocable_r_v<int, const FR&, int>);  // operator() is const
static_assert(!std::is_nothrow_invocable_v<const FR&, int>);
static_assert(std::is_nothrow_invocable_v<const std::function_ref<int(int) noexcept>&, int>);

int main() {
  FR a(&twice);
  CHECK(a(4) == 8);
  FR b(twice);  // function lvalue
  CHECK(b(5) == 10);
  CHECK(sum_with(twice, 4) == 12);
  // a temporary callable lives until the end of the full-expression
  CHECK(sum_with([](int x) { return x * x; }, 4) == 14);

  // the callable is referenced, not copied
  Stateful st;
  FR s(st);
  s(3);
  s(4);
  CHECK(st.n == 7);

  // cv of the signature selects the overload: cv T&
  Both both;
  std::function_ref<int()> nc(both);
  std::function_ref<int() const> c(both);
  CHECK(nc() == 1 && c() == 2);
  const Both cboth;
  std::function_ref<int() const> c2(cboth);
  CHECK(c2() == 2);

  // copy and assignment rebind
  FR copy = s;
  copy(1);
  CHECK(st.n == 8);
  copy = a;
  CHECK(copy(1) == 2);
  copy = &twice;  // assignment from a function pointer goes through function_ref(F*)
  CHECK(copy(3) == 6);

  // INVOKE<R>: void discards, conversions apply
  int hits = 0;
  auto inc = [&](int k) { return hits += k; };
  std::function_ref<void(int)> v(inc);
  v(2);
  CHECK(hits == 2);
  std::function_ref<long(short)> conv(twice);
  CHECK(conv(short(21)) == 42L);

  // arguments are forwarded per the signature
  auto set = [](int& r) { r = 5; };
  std::function_ref<void(int&)> sr(set);
  int x = 0;
  sr(x);
  CHECK(x == 5);
  auto take = [](int&& r) { return r + 1; };
  std::function_ref<int(int&&)> tr(take);
  CHECK(tr(1) == 2);

  // noexcept signature
  auto ne = [](int i) noexcept { return i; };
  std::function_ref<int(int) noexcept> nr(ne);
  CHECK(nr(9) == 9);
  return 0;
}
