// [func.bind.partial]/4: bind_front/bind_back return a perfect forwarding call wrapper with
// call pattern invoke(fd, bound_args..., call_args...) / invoke(fd, call_args...,
// bound_args...); /1.4-1.5: bound_args have types decay_t<Args>... [func.require]/4: state
// entities are delivered as cv T& for an lvalue wrapper and cv T&& otherwise; /5: call
// arguments are forwarded; /8: same state entity types give the same wrapper type.
// Unlike bind, a bound reference_wrapper is passed on as is (invoke unwraps it only as the
// object argument of a member pointer, [func.require]/1).
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

template <class T>
constexpr int code() {  // 1: T&  2: const T&  3: T&&  4: const T&&
  constexpr bool c = std::is_const_v<std::remove_reference_t<T>>;
  return std::is_lvalue_reference_v<T> ? (c ? 2 : 1) : (c ? 4 : 3);
}
struct Cat2 {
  template <class A, class B>
  constexpr int operator()(A&&, B&&) const { return code<A&&>() * 10 + code<B&&>(); }
};
struct IsRefWrap {
  template <class T>
  constexpr bool operator()(T&&) const {
    return std::is_same_v<std::remove_cvref_t<T>, std::reference_wrapper<int>>;
  }
};
struct S {
  int v;
  constexpr int get() const { return v; }
};
constexpr int sub(int a, int b) { return a - b; }
constexpr int sub3(int a, int b, int c) { return a - b - c; }
constexpr int& id_ref(int& r) { return r; }
constexpr int first_char(const char* s) { return s[0]; }

constexpr bool test() {
  int x = 0;
  const int cx = 0;
  // bound argument (state entity) and call argument categories, front and back
  auto f = std::bind_front(Cat2{}, 0);
  const auto& cf = f;
  if (f(x) != 11 || f(cx) != 12 || f(std::move(x)) != 13 || f(std::move(cx)) != 14) return false;
  if (cf(x) != 21 || std::move(f)(x) != 31 || std::move(cf)(std::move(x)) != 43) return false;
  auto b = std::bind_back(Cat2{}, 0);
  const auto& cb = b;
  if (b(x) != 11 || b(std::move(cx)) != 41 || cb(x) != 12 || std::move(b)(cx) != 23) return false;
  if (std::move(cb)(std::move(x)) != 34) return false;
  // reference_wrapper bound arguments are not unwrapped
  if (!std::bind_front(IsRefWrap{}, std::ref(x))()) return false;
  if (!std::bind_back(IsRefWrap{}, std::ref(x))()) return false;
  // ...but convert where the target needs a reference
  auto r = std::bind_front(id_ref, std::ref(x));
  r() = 5;
  if (x != 5) return false;
  // invoke unwraps a reference_wrapper used as the object of a member pointer
  S s{1};
  auto g = std::bind_front(&S::get, std::ref(s));
  s.v = 2;
  if (g() != 2) return false;
  // pointer to data member: the state entity's category reaches the result
  auto d = std::bind_front(&S::v, S{3});
  if (d() != 3 || std::move(d)() != 3) return false;
  // nesting and ordering
  if (std::bind_front(std::bind_back(sub, 1), 10)() != 9) return false;
  if (std::bind_back(sub3, 1)(10, 2) != 7) return false;
  if (std::bind_front(sub3, 10)(2, 1) != 7) return false;
  // decay of bound arguments: an array becomes a pointer
  if (std::bind_front(first_char, "abc")() != 'a') return false;
  return true;
}
static_assert(test());

using D = decltype(std::bind_front(&S::v, S{3}));
static_assert(std::is_same_v<decltype(std::declval<D&>()()), int&>);
static_assert(std::is_same_v<decltype(std::declval<const D&>()()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<D>()()), int&&>);
static_assert(std::is_same_v<decltype(std::declval<const D>()()), const int&&>);
using DB = decltype(std::bind_back(&S::v));
static_assert(std::is_same_v<decltype(std::declval<DB&>()(std::declval<S>())), int&&>);
static_assert(std::is_same_v<decltype(std::declval<DB&>()(std::declval<const S&>())), const int&>);
static_assert(std::is_same_v<decltype(std::bind_front(id_ref, std::ref(std::declval<int&>()))()), int&>);
// decayed state entity types decide the wrapper type
static_assert(std::is_same_v<decltype(std::bind_front(sub, 1)), decltype(std::bind_front(&sub, 1))>);
static_assert(std::is_same_v<decltype(std::bind_front(first_char, "abc")),
                             decltype(std::bind_front(first_char, static_cast<const char*>(nullptr)))>);
static_assert(std::is_same_v<decltype(std::bind_back(sub, 1)), decltype(std::bind_back(sub, std::declval<const int&>()))>);

int main() {
  CHECK(test());
  return 0;
}
