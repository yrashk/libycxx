// [func.wrap.ref.ctor]/2: is-convertible-from-specialization<F> is true when F is
// function_ref<R(Args...) cv2 noexcept(noex2)> (same R and Args) with
// is_convertible_v<R(&)(Args...) noexcept(noex2), R(&)(Args...) noexcept(noex)> &&
// is_convertible_v<int cv&, int cv2&>. /8: function_ref(F&& f): "If
// is-convertible-from-specialization<remove_cv_t<T>> is false, initializes bound-entity with
// addressof(f) ... Otherwise, initializes bound-entity with the value of f.bound-entity and
// thunk-ptr with the value of f.thunk-ptr." So the new function_ref refers to what f referred
// to, not to f itself: rebinding or destroying f afterwards does not affect it.
// COUNTERPART: libstdcxx:20_util/function_ref/conv.cc
#include <functional>
#include <type_traits>
#include "check.hpp"

int one() { return 1; }
int two() { return 2; }
int one_nx() noexcept { return 1; }
int two_nx() noexcept { return 2; }
struct Obj {
  int v;
  int operator()() const { return v; }
};

std::function_ref<int()> outlive() {
  std::function_ref<int() const noexcept> tmp(&one_nx);
  return std::function_ref<int()>(tmp);  // tmp is destroyed; the result refers to one_nx
}

int main() {
  // const -> non-const
  std::function_ref<int() const> rc(&one);
  std::function_ref<int()> r(rc);
  rc = &two;
  CHECK(r() == 1);
  CHECK(rc() == 2);
  // noexcept -> non-noexcept
  std::function_ref<int() noexcept> rn(&one_nx);
  std::function_ref<int()> r2(rn);
  rn = &two_nx;
  CHECK(r2() == 1);
  // const noexcept -> non-const non-noexcept, from a const lvalue
  const std::function_ref<int() const noexcept> rcn(&one_nx);
  std::function_ref<int()> r3(rcn);
  CHECK(r3() == 1);
  // the source no longer exists
  CHECK(outlive()() == 1);
  // a bound object: the new function_ref refers to the object, not to the source function_ref
  Obj a{10}, b{20};
  std::function_ref<int() const> oa(a);
  std::function_ref<int()> ob(oa);
  oa = std::function_ref<int() const>(b);
  CHECK(ob() == 10 && oa() == 20);
  a.v = 11;
  CHECK(ob() == 11);
  // assignment from a convertible specialization goes through the same constructor
  std::function_ref<int()> as(&two);
  std::function_ref<int() const> src(a);
  as = src;
  src = std::function_ref<int() const>(b);
  CHECK(as() == 11);
  return 0;
}

static_assert(std::is_nothrow_constructible_v<std::function_ref<int()>, std::function_ref<int() const>&>);
static_assert(std::is_nothrow_constructible_v<std::function_ref<int()>, std::function_ref<int() noexcept>>);
static_assert(std::is_convertible_v<std::function_ref<int() const noexcept>, std::function_ref<int()>>);
static_assert(!std::is_constructible_v<std::function_ref<int() noexcept>, std::function_ref<int()>>);
