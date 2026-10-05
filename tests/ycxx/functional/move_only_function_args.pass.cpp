// [func.wrap.move.inv]/3: "return INVOKE<R>(static_cast<F inv-quals>(f),
// std::forward<ArgTypes>(args)...);" -- arguments are forwarded according to the signature,
// the result is converted to R (or discarded for void), and member pointers are invoked
// through INVOKE ([func.require]).
// REQUIRES: exceptions
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct MoveOnlyArg {
  int v;
  explicit MoveOnlyArg(int x) : v(x) {}
  MoveOnlyArg(MoveOnlyArg&& o) noexcept : v(o.v) { o.v = -1; }
  MoveOnlyArg(const MoveOnlyArg&) = delete;
};
struct S {
  int v;
  int scaled(int k) const { return v * k; }
  int& ref() { return v; }
};
struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

static_assert(std::is_invocable_r_v<int, std::move_only_function<int(MoveOnlyArg)>&, MoveOnlyArg>);
static_assert(!std::is_invocable_v<std::move_only_function<int(MoveOnlyArg)>&, MoveOnlyArg&>);
static_assert(std::is_same_v<std::invoke_result_t<std::move_only_function<int&(S&)>&, S&>, int&>);

int main() {
  // by-value parameter of a move-only type
  std::move_only_function<int(MoveOnlyArg)> byval = [](MoveOnlyArg a) { return a.v; };
  CHECK(byval(MoveOnlyArg(4)) == 4);
  // rvalue-reference parameter: the argument is forwarded as an rvalue, not copied
  std::move_only_function<int(MoveOnlyArg&&)> byrref = [](MoveOnlyArg&& a) {
    MoveOnlyArg sink(std::move(a));
    return sink.v;
  };
  MoveOnlyArg arg(9);
  CHECK(byrref(std::move(arg)) == 9);
  CHECK(arg.v == -1);
  // lvalue-reference parameter: the target sees the caller's object
  std::move_only_function<void(int&)> inc = [](int& x) { ++x; };
  int i = 1;
  inc(i);
  CHECK(i == 2);
  // reference result is preserved
  std::move_only_function<int&(S&)> getref = &S::ref;
  S s{3};
  getref(s) = 10;
  CHECK(s.v == 10);
  // member function and data member pointers
  std::move_only_function<int(const S&, int)> mf = &S::scaled;
  CHECK(mf(S{2}, 21) == 42);
  std::move_only_function<int(const S*, int)> mfp = &S::scaled;
  CHECK(mfp(&s, 2) == 20);
  std::move_only_function<int(S)> md = &S::v;
  CHECK(md(S{5}) == 5);
  // INVOKE<R>: void discards, conversions apply
  int calls = 0;
  std::move_only_function<void()> v = [&] { return ++calls; };
  v();
  CHECK(calls == 1);
  std::move_only_function<long(short)> conv = [](int x) { return x * 2; };
  CHECK(conv(short(21)) == 42L);
  Derived d;
  std::move_only_function<Base*()> up = [&] { return &d; };
  CHECK(up() == &d);
  // exceptions from the target propagate
  std::move_only_function<void()> thrower = [] { throw 3; };
  int caught = 0;
  try {
    thrower();
  } catch (int e) {
    caught = e;
  }
  CHECK(caught == 3);
  return 0;
}
