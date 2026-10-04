// [func.wrap.ref.class]/3: call-args are such that decltype((call-args))... are ArgTypes&&...;
// [func.wrap.ref.inv]/1: operator() "Equivalent to: return thunk-ptr(bound-entity,
// std::forward<ArgTypes>(args)...);" and the thunk is invoke_r<R>(..., call-args...) -- so
// by-value parameters are moved into the target, reference parameters keep their category, the
// result is converted to R (discarded for void), and exceptions from the target propagate.
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct MoveOnly {
  int v;
  explicit MoveOnly(int x) : v(x) {}
  MoveOnly(MoveOnly&& o) noexcept : v(o.v) { o.v = -1; }
  MoveOnly(const MoveOnly&) = delete;
};
struct Probe {
  int operator()(int&) const { return 1; }
  int operator()(const int&) const { return 2; }
  int operator()(int&&) const { return 3; }
};
struct Conv {
  int v;
  Conv(int x) : v(x) {}
};

int main() {
  // by-value move-only parameter: moved into the target
  auto take = [](MoveOnly m) { return m.v; };
  std::function_ref<int(MoveOnly)> t(take);
  CHECK(t(MoveOnly(4)) == 4);
  MoveOnly src(5);
  CHECK(t(std::move(src)) == 5);
  // by-value parameter reaches the target as an rvalue (std::forward<ArgTypes>)
  Probe p;
  std::function_ref<int(int)> byval(p);
  int x = 0;
  CHECK(byval(x) == 3);
  std::function_ref<int(int&)> lref(p);
  CHECK(lref(x) == 1);
  std::function_ref<int(const int&)> clref(p);
  CHECK(clref(x) == 2);
  std::function_ref<int(int&&)> rref(p);
  CHECK(rref(0) == 3);
  // argument conversion happens at the call to the function_ref
  auto get = [](Conv c) { return c.v; };
  std::function_ref<int(Conv)> conv(get);
  CHECK(conv(7) == 7);
  // reference results alias, other results are converted
  int y = 1;
  auto refy = [&y]() -> int& { return y; };
  std::function_ref<int&()> ry(refy);
  ry() = 9;
  CHECK(y == 9);
  std::function_ref<const int&()> cry(refy);
  CHECK(&cry() == &y);
  std::function_ref<double()> dr(refy);
  CHECK(dr() == 9.0);
  std::function_ref<void()> vr(refy);
  vr();
  // exceptions propagate
  auto thrower = [](int v) -> int { throw v; };
  std::function_ref<int(int)> th(thrower);
  bool caught = false;
  try {
    th(3);
  } catch (int v) {
    caught = v == 3;
  }
  CHECK(caught);
  return 0;
}

static_assert(std::is_same_v<decltype(std::declval<std::function_ref<int&()>>()()), int&>);
static_assert(std::is_same_v<decltype(std::declval<std::function_ref<void(int)>>()(1)), void>);
static_assert(std::is_invocable_v<std::function_ref<int(long)>, int>);
static_assert(!std::is_invocable_v<std::function_ref<int(int&)>, int>);
static_assert(std::is_nothrow_invocable_v<std::function_ref<int(int) const noexcept>&, int>);
