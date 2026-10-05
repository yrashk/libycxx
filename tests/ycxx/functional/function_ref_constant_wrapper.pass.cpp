// [func.wrap.ref.ctor]/10-12: function_ref(constant_wrapper<c, F> f): calls are
// invoke_r<R>(f.value, call-args...). /13-16: function_ref(constant_wrapper<c, F> f, U&& obj)
// binds addressof(obj); calls are invoke_r<R>(f.value, static_cast<cv T&>(obj),
// call-args...). /17-20: function_ref(constant_wrapper<c, F> f, cv T* obj) binds obj; calls are
// invoke_r<R>(f.value, obj, call-args...).
// COUNTERPART: libstdcxx:20_util/constant_wrapper/instantiate.cc
#include <functional>
#include <utility>
#include "check.hpp"

int twice(int x) { return 2 * x; }
struct S {
  int v;
  int get() const { return v; }
  int scaled(int k) const { return v * k; }
  int bump(int k) { return v += k; }
};
struct Which {  // an empty, structural callable object usable as a constant_wrapper value
  constexpr int operator()(S&) const { return 1; }
  constexpr int operator()(const S&) const { return 2; }
};
constexpr Which which{};
int add_to(S& s, int k) { return s.v += k; }
int peek(const S* s, int k) { return s->v + k; }

int main() {
  std::function_ref<int(int)> a(std::cw<twice>);
  CHECK(a(21) == 42);
  std::function_ref<long(int)> a2 = std::cw<&twice>;
  CHECK(a2(1) == 2L);
  constexpr auto sq = [](int x) { return x * x; };
  std::function_ref<int(int)> lam(std::cw<sq>);
  CHECK(lam(5) == 25);

  // bound object by reference
  S s{3};
  std::function_ref<int()> g(std::cw<&S::get>, s);
  CHECK(g() == 3);
  s.v = 4;
  CHECK(g() == 4);  // refers to s, not a copy
  std::function_ref<int(int)> sc(std::cw<&S::scaled>, s);
  CHECK(sc(10) == 40);
  std::function_ref<int(int)> bp(std::cw<&S::bump>, s);
  bp(1);
  CHECK(s.v == 5);
  std::function_ref<int(int)> ft(std::cw<add_to>, s);  // free function, first argument bound
  ft(5);
  CHECK(s.v == 10);
  // cv T& selects the overload
  std::function_ref<int()> w1(std::cw<which>, s);
  std::function_ref<int() const> w2(std::cw<which>, s);
  CHECK(w1() == 1 && w2() == 2);
  // data member
  std::function_ref<int&()> dm(std::cw<&S::v>, s);
  dm() = 11;
  CHECK(s.v == 11);

  // bound object by pointer
  std::function_ref<int()> gp(std::cw<&S::get>, &s);
  CHECK(gp() == 11);
  s.v = 12;
  CHECK(gp() == 12);
  const S cs{20};
  std::function_ref<int(int)> pk(std::cw<peek>, &cs);
  CHECK(pk(1) == 21);
  std::function_ref<int(int)> bpp(std::cw<&S::bump>, &s);
  bpp(3);
  CHECK(s.v == 15);
  return 0;
}
