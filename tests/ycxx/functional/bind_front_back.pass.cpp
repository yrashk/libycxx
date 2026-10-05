// [func.bind.partial]/1-5: bind_front(f, args...) / bind_back(f, args...) return a perfect
// forwarding call wrapper with call pattern invoke(fd, bound_args..., call_args...) /
// invoke(fd, call_args..., bound_args...); bound arguments are decay-copied.
// /6-10 (C++26, P2714): template<auto f, class... Args> bind_front(args...) / bind_back.
// COUNTERPART: libcxx:utilities/function.objects/func.bind_front/bind_front.pass.cpp
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

constexpr int sub(int a, int b) { return a - b; }
constexpr int sub3(int a, int b, int c) { return a - b - c; }
struct S {
  int v;
  constexpr int add(int x) const { return v + x; }
};
struct Probe {
  constexpr int operator()(int&) const { return 1; }
  constexpr int operator()(int&&) const { return 2; }
  constexpr int operator()(const int&) const { return 3; }
  constexpr int operator()(const int&&) const { return 4; }
};

constexpr bool test() {
  if (std::bind_front(sub, 10)(3) != 7) return false;
  if (std::bind_back(sub, 10)(3) != -7) return false;
  if (std::bind_front(sub3, 10, 1)(2) != 7) return false;
  if (std::bind_back(sub3, 1, 2)(10) != 7) return false;
  if (std::bind_front(sub3)(10, 1, 2) != 7) return false;
  S s{5};
  if (std::bind_front(&S::add, s)(1) != 6) return false;
  if (std::bind_front(&S::add, &s)(2) != 7) return false;
  if (std::bind_back(&S::add, 3)(s) != 8) return false;
  // bound arguments are copies; use std::ref to bind by reference
  int x = 1;
  auto byval = std::bind_front([](int& r) { return ++r; }, x);
  byval();
  if (x != 1) return false;
  auto byref = std::bind_front([](int& r) { return ++r; }, std::ref(x));
  byref();
  if (x != 2) return false;
  // the bound argument is passed as an lvalue/rvalue/const according to the wrapper
  auto p = std::bind_front(Probe{}, 0);
  const auto& cp = p;
  if (p() != 1 || std::move(p)() != 2 || cp() != 3 || std::move(cp)() != 4) return false;
  auto q = std::bind_back(Probe{}, 0);
  if (q() != 1 || std::move(q)() != 2) return false;
  // C++26 forms with a constant target
  if (std::bind_front<sub>(10)(4) != 6) return false;
  if (std::bind_back<sub>(10)(4) != -6) return false;
  if (std::bind_front<&S::add>(s)(10) != 15) return false;
  if (std::bind_back<&S::add>(10)(s) != 15) return false;
  if (std::bind_front<sub3>(10, 1)(2) != 7) return false;
  return true;
}
static_assert(test());

static_assert(std::is_empty_v<decltype(std::bind_front<sub>())>);
static_assert(std::is_same_v<decltype(std::bind_front(sub, 1)(2)), int>);

int main() {
  CHECK(test());
  return 0;
}
