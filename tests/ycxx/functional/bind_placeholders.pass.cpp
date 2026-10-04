// [func.bind.bind]/7.3: "if the value j of is_placeholder_v<TDi> is not zero, the argument is
// std::forward<Uj>(uj) and its type Vi is Uj&&" -- so a call argument used twice is forwarded
// twice with the same category; /7.4 bound values are cv TDi& lvalues. /1.5: TDi is decay_t<Ti>,
// so a placeholder passed as a const lvalue is still a placeholder. [func.bind.place]: _1 ... _M
// (_1 to _9 are used here).
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

using namespace std::placeholders;

template <class T>
constexpr int code() {  // 1: T&  2: const T&  3: T&&  4: const T&&
  constexpr bool c = std::is_const_v<std::remove_reference_t<T>>;
  return std::is_lvalue_reference_v<T> ? (c ? 2 : 1) : (c ? 4 : 3);
}
struct Cat2 {
  template <class A, class B>
  constexpr int operator()(A&&, B&&) const { return code<A&&>() * 10 + code<B&&>(); }
};
constexpr int digits(int a, int b, int c, int d, int e, int f, int g, int h, int i) {
  return ((((((((a * 10 + b) * 10 + c) * 10 + d) * 10 + e) * 10 + f) * 10 + g) * 10 + h) * 10) + i;
}
constexpr int sub(int a, int b) { return a - b; }

constexpr bool test() {
  int x = 0;
  const int cx = 0;
  if (std::bind(Cat2{}, _1, _1)(1) != 33) return false;
  if (std::bind(Cat2{}, _1, _1)(x) != 11) return false;
  if (std::bind(Cat2{}, _1, _2)(cx, std::move(cx)) != 24) return false;
  if (std::bind(Cat2{}, _2, _1)(cx, std::move(x)) != 32) return false;
  // a bound value and a placeholder; the wrapper's constness reaches only the bound value
  auto g = std::bind(Cat2{}, 0, _1);
  const auto& cg = g;
  if (g(1) != 13 || cg(1) != 23 || std::move(g)(x) != 11 || std::move(cg)(cx) != 22) return false;
  // all nine standard placeholders
  if (std::bind(digits, _9, _8, _7, _6, _5, _4, _3, _2, _1)(1, 2, 3, 4, 5, 6, 7, 8, 9) != 987654321) return false;
  if (std::bind(digits, _1, _2, _3, _4, _5, _6, _7, _8, _9)(1, 2, 3, 4, 5, 6, 7, 8, 9) != 123456789) return false;
  // placeholders given as const lvalues decay to the placeholder type
  const auto& p2 = _2;
  if (std::bind(sub, p2, _1)(3, 10) != 7) return false;
  return true;
}
static_assert(test());

// a placeholder may be copied and the copy still acts as a placeholder
constexpr auto my1 = _1;
static_assert(std::bind(sub, my1, 1)(5) == 4);
static_assert(std::is_same_v<decltype(std::bind(Cat2{}, _1, 0)(1)), int>);

int main() {
  CHECK(test());
  return 0;
}
