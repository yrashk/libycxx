// [func.wrap.move.inv]/3: operator() is "Equivalent to: return INVOKE<R>(static_cast<F
// inv-quals>(f), std::forward<ArgTypes>(args)...);" where inv-quals is cv& if ref is empty and
// cv ref otherwise ([func.wrap.move.general]/2). So the target sees exactly the wrapper's
// cv-qualification, and an rvalue only for && signatures.
#include <functional>
#include <utility>
#include "check.hpp"

struct Q {
  int operator()() & { return 1; }
  int operator()() const& { return 2; }
  int operator()() && { return 3; }
  int operator()() const&& { return 4; }
};
struct QN {
  int operator()() & noexcept { return 1; }
  int operator()() const& noexcept { return 2; }
  int operator()() && noexcept { return 3; }
  int operator()() const&& noexcept { return 4; }
};

template <class Sig, class T>
void check_all(int lv, int clv, int rv, int crv) {
  std::move_only_function<Sig> f(T{});
  if constexpr (requires { f(); }) CHECK(f() == lv);
  if constexpr (requires { std::as_const(f)(); }) CHECK(std::as_const(f)() == clv);
  if constexpr (requires { std::move(f)(); }) CHECK(std::move(f)() == rv);
  if constexpr (requires { std::move(std::as_const(f))(); }) CHECK(std::move(std::as_const(f))() == crv);
}

int main() {
  // signature            lvalue const-lvalue rvalue const-rvalue
  check_all<int(), Q>(1, -1, 1, -1);
  check_all<int() const, Q>(2, 2, 2, 2);
  check_all<int() &, Q>(1, -1, -1, -1);
  check_all<int() const&, Q>(2, 2, 2, 2);
  check_all<int() &&, Q>(-1, -1, 3, -1);
  check_all<int() const&&, Q>(-1, -1, 4, 4);
  check_all<int() noexcept, QN>(1, -1, 1, -1);
  check_all<int() const noexcept, QN>(2, 2, 2, 2);
  check_all<int() & noexcept, QN>(1, -1, -1, -1);
  check_all<int() const & noexcept, QN>(2, 2, 2, 2);
  check_all<int() && noexcept, QN>(-1, -1, 3, -1);
  check_all<int() const && noexcept, QN>(-1, -1, 4, 4);

  // a mutable lambda keeps its state across calls through a non-const wrapper
  std::move_only_function<int()> counter = [n = 0]() mutable { return ++n; };
  CHECK(counter() == 1 && counter() == 2 && std::move(counter)() == 3);
  // && signature: the target is called as an rvalue and may consume its state
  struct Consume {
    int v = 7;
    int operator()() && { return std::exchange(v, 0); }
  };
  std::move_only_function<int() &&> once = Consume{};
  CHECK(std::move(once)() == 7);
  CHECK(std::move(once)() == 0);
  return 0;
}
