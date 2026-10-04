// [coro.generator.promise]/11: the awaitable of co_yield elements_of(g) "pushes
// g.range.coroutine_ into *x.active_ and resumes execution of the coroutine", and its
// await_resume "evaluates rethrow_exception(except_) if bool(except_) is true"; /16:
// unhandled_exception() stores current_exception() when the coroutine is not the sole element
// of the stack, and is "throw;" when it is. So a nested generator that throws before yielding
// anything (also two levels down) makes the co_yield in its parent throw; the parent may catch
// and go on. If the root itself ends with that exception, it propagates from operator++ (or
// from begin() when no value was produced yet) and the root is then at its final suspend
// point (it == end(), [dcl.fct.def.coroutine]/14). Every frame's locals are destroyed.
#include <generator>
#include <ranges>
#include <stdexcept>
#include <vector>
#include "check.hpp"

static int live = 0;
struct Guard {
  Guard() { ++live; }
  Guard(const Guard&) = delete;
  ~Guard() { --live; }
};

std::generator<int> boom() {
  Guard g;
  throw std::runtime_error("boom");
  co_return;
}
std::generator<int> boom2() {
  Guard g;
  co_yield std::ranges::elements_of(boom());
  co_yield 5;  // not reached
}
std::generator<int> parent() {
  Guard g;
  co_yield 1;
  bool caught = false;
  try {
    co_yield std::ranges::elements_of(boom2());
  } catch (const std::runtime_error&) {
    caught = true;
  }
  co_yield caught ? 2 : -2;
  co_yield std::ranges::elements_of(boom());
  co_yield 3;  // not reached
}

int main() {
  std::vector<int> got;
  bool threw = false;
  {
    auto g = parent();
    auto it = g.begin();
    try {
      for (; it != g.end(); ++it) got.push_back(*it);
    } catch (const std::runtime_error&) {
      threw = true;
    }
    CHECK(threw && it == g.end() && live == 0);
  }
  CHECK((got == std::vector<int>{1, 2}));
  threw = false;
  try {
    auto g = boom2();
    (void)g.begin();
  } catch (const std::runtime_error&) {
    threw = true;
  }
  CHECK(threw && live == 0);
  return 0;
}
