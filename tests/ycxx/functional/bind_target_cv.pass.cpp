// [func.bind.bind]/8: "The value of the target argument vfd is fd and its corresponding type
// Vfd is cv FD&." -- the target is always passed as an lvalue whose constness is that of
// the call wrapper. /7.2: a nested bind expression is called as static_cast<cv TDi&>(tdi)(...).
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

using namespace std::placeholders;

struct Target {
  int operator()() & { return 1; }
  int operator()() const& { return 2; }
  int operator()() && { return 3; }
  int operator()() const&& { return 4; }
};
struct Inner {
  int operator()(int) & { return 10; }
  int operator()(int) const& { return 20; }
};

int main() {
  auto g = std::bind(Target{});
  const auto& cg = g;
  CHECK(g() == 1);
  CHECK(cg() == 2);
  CHECK(std::move(g)() == 1);   // still an lvalue target
  CHECK(std::move(cg)() == 2);  // const lvalue target
  auto n = std::bind([](int v) { return v; }, std::bind(Inner{}, _1));
  const auto& cn = n;
  CHECK(n(0) == 10);
  CHECK(cn(0) == 20);
  return 0;
}
