// [func.bind.bind]/7.2: "if the value of is_bind_expression_v<TDi> is true, the argument is
// static_cast<cv TDi&>(tdi)(std::forward<Uj>(uj)...) and its type Vi is
// invoke_result_t<cv TDi&, Uj...>&&" -- the nested bind expression receives all call
// arguments (forwarded), and its result reaches the outer target as an xvalue when it is a
// prvalue and as the returned reference otherwise. /7.1 takes precedence: a bound
// reference_wrapper to a bind expression passes the bind expression itself. /8: the target
// (even if it is a bind expression) is just called as vfd.
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
constexpr int value(int x) { return x; }
constexpr int& ref(int& r) { return r; }
constexpr const int& cref(const int& r) { return r; }
constexpr int sub(int a, int b) { return a - b; }
constexpr int twice(int x) { return 2 * x; }
constexpr int pass(int x) { return x; }

constexpr bool test() {
  int x = 0;
  // the category of the nested result
  if (std::bind(Probe{}, std::bind(value, _1))(5) != 3) return false;
  if (std::bind(Probe{}, std::bind(ref, _1))(x) != 1) return false;
  if (std::bind(Probe{}, std::bind(cref, _1))(x) != 2) return false;
  // the nested expression receives the call arguments with their categories
  if (std::bind(pass, std::bind(Probe{}, _1))(x) != 1) return false;
  if (std::bind(pass, std::bind(Probe{}, _1))(std::move(x)) != 3) return false;
  if (std::bind(pass, std::bind(Probe{}, _2))(0, std::as_const(x)) != 2) return false;
  // the nested expression sees every call argument, including ones the outer one ignores
  if (std::bind(sub, std::bind(sub, _2, _1), _3)(1, 10, 2) != 7) return false;
  // several nested expressions and nesting depth
  if (std::bind(sub, std::bind(twice, _1), std::bind(pass, _2))(5, 3) != 7) return false;
  if (std::bind(twice, std::bind(sub, std::bind(twice, _2), _1))(1, 3) != 10) return false;
  // a bind expression as the target is not evaluated first: it is invoked with the bound args
  if (std::bind(std::bind(sub, _1, _2), _2, _1)(3, 10) != 7) return false;
  // bind<R> nested
  if (std::bind<long>(sub, std::bind<int>(twice, _1), 1)(4) != 7L) return false;
  // a nested bind expression is evaluated on every call
  int calls = 0;
  auto counting = std::bind(pass, std::bind([&calls](int v) { ++calls; return v; }, _1));
  counting(1);
  counting(2);
  if (calls != 2) return false;
  // reference_wrapper to a bind expression: /7.1 applies, the expression itself is passed
  auto inner = std::bind(twice, _1);
  auto apply5 = [](auto& b) { return b(5); };
  if (std::bind(apply5, std::ref(inner))() != 10) return false;
  return true;
}
static_assert(test());

static_assert(std::is_same_v<decltype(std::bind(ref, std::bind(ref, _1))(std::declval<int&>())), int&>);

int main() {
  CHECK(test());
  return 0;
}
