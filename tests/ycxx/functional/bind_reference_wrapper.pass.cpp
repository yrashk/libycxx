// [func.bind.bind]/7.1: "if TDi is reference_wrapper<T>, the argument is tdi.get() and its type
// Vi is T&" -- independently of the cv-qualification and value category of the call wrapper.
// /7.3: a placeholder passes std::forward<Uj>(uj), so a reference_wrapper given as a call
// argument is not unwrapped. /1.3, /8: a reference_wrapper target is stored as such and the
// referenced object is called.
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

using namespace std::placeholders;

struct Probe {
  constexpr int operator()(int&) const { return 1; }
  constexpr int operator()(const int&) const { return 2; }
  constexpr int operator()(int&&) const { return 3; }
  constexpr int operator()(const int&&) const { return 4; }
};
struct IsRefWrap {
  template <class T>
  constexpr bool operator()(T&&) const {
    return std::is_same_v<std::remove_cvref_t<T>, std::reference_wrapper<int>>;
  }
};
struct Counter {
  int n = 0;
  constexpr int operator()() { return ++n; }
};
struct S {
  int v;
  constexpr int get() const { return v; }
  constexpr void set(int x) { v = x; }
};
constexpr int& id(int& r) { return r; }

constexpr bool test() {
  int x = 0;
  auto g = std::bind(Probe{}, std::ref(x));
  const auto& cg = g;
  if (g() != 1 || cg() != 1 || std::move(g)() != 1 || std::move(cg)() != 1) return false;
  auto h = std::bind(Probe{}, std::cref(x));
  if (h() != 2 || std::move(h)() != 2) return false;
  // the bound reference_wrapper is unwrapped before the call
  if (std::bind(IsRefWrap{}, std::ref(x))()) return false;
  // but one arriving through a placeholder is forwarded unchanged
  if (!std::bind(IsRefWrap{}, _1)(std::ref(x))) return false;
  // the referenced object, not a copy, is used
  auto r = std::bind(id, std::ref(x));
  r() = 42;
  if (x != 42) return false;
  // reference_wrapper target
  Counter c;
  auto t = std::bind(std::ref(c));
  t();
  t();
  auto t2 = t;  // copies the reference_wrapper, not the counter
  t2();
  if (c.n != 3) return false;
  // member pointer with a reference_wrapper object
  S s{1};
  auto set = std::bind(&S::set, std::ref(s), _1);
  set(9);
  if (s.v != 9) return false;
  auto get = std::bind(&S::get, std::cref(s));
  s.v = 10;
  if (get() != 10) return false;
  return true;
}
static_assert(test());

static_assert(std::is_same_v<decltype(std::bind(id, std::ref(std::declval<int&>()))()), int&>);
static_assert(std::is_invocable_v<decltype(std::bind(&S::get, std::cref(std::declval<S&>())))>);
static_assert(!std::is_invocable_v<decltype(std::bind(&S::set, std::cref(std::declval<S&>()), 1))>);

int main() {
  CHECK(test());
  return 0;
}
