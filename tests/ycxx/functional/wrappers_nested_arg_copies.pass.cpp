// [func.wrap.general]/2: when the target object x of a function / copyable_function /
// move_only_function / function_ref t is itself such a wrapper, "Each argument of the
// invocation of x evaluated as part of the invocation of t may alias an argument in the same
// position in the invocation of t that has the same type, even if the corresponding parameter
// is not of reference type." [Example 1: move_only_function<void(T)> f{copyable_function<
// void(T)>{[](T) {}}}; T t; f(t); // it is unspecified how many copies of T are made]
// /3: implementations should avoid double wrapping.
// What stays specified: the outer call copy-initializes its by-value parameter from the
// caller's argument (one copy from an lvalue, none from a prvalue), and each wrapper passes
// std::forward<ArgTypes>(args)... on ([func.wrap.func.inv]/2, [func.wrap.move.inv]/3,
// [func.wrap.copy.inv]/3, [func.wrap.ref.inv]/2), i.e. an rvalue that can only be moved from.
// So however the wrappers are nested, an lvalue argument is copied exactly once and an
// rvalue argument never, and the innermost callable sees the caller's value.
#include <functional>
#include <utility>
#include "check.hpp"

struct Arg {
  static inline int copies = 0;
  int v;
  explicit Arg(int x) : v(x) {}
  Arg(const Arg& o) : v(o.v) { ++copies; }
  Arg(Arg&& o) noexcept : v(o.v) {}
};
using Sig = void(Arg);

int seen = 0;
auto by_value = [](Arg a) { seen = a.v; };
auto by_cref = [](const Arg& a) { seen = a.v; };

template <class F>
void check(F& f, int v) {
  Arg a(v);
  Arg::copies = 0;
  seen = 0;
  f(a);
  CHECK(seen == v);
  CHECK(Arg::copies == 1);
  Arg::copies = 0;
  seen = 0;
  f(Arg(v + 1));
  CHECK(seen == v + 1);
  CHECK(Arg::copies == 0);
  Arg::copies = 0;
  seen = 0;
  f(std::move(a));
  CHECK(seen == v);
  CHECK(Arg::copies == 0);
}

int main() {
  {  // the draft's example
    std::move_only_function<Sig> f{std::copyable_function<Sig>{by_value}};
    check(f, 10);
    std::move_only_function<Sig> g{std::copyable_function<Sig>{by_cref}};
    check(g, 11);
  }
  {
    std::move_only_function<Sig> f{std::move_only_function<void(Arg) const>{by_value}};
    check(f, 20);
    std::move_only_function<Sig> g{std::function<Sig>{by_value}};
    check(g, 21);
  }
  {
    std::copyable_function<Sig> f{std::function<Sig>{by_value}};
    check(f, 30);
    std::copyable_function<Sig> g{std::copyable_function<void(Arg) const>{by_value}};
    check(g, 31);
  }
  {
    std::function<Sig> f{std::copyable_function<Sig>{by_value}};
    check(f, 40);
    std::copyable_function<Sig> inner{by_value};
    std::function<Sig> g{std::function_ref<Sig>{inner}};
    check(g, 41);
  }
  {
    std::move_only_function<Sig> m{by_value};
    std::function_ref<Sig> r{m};
    check(r, 50);
    std::function_ref<Sig> rr{r};  // function_ref of the same signature
    check(rr, 51);
    std::function<Sig> fn{by_value};
    std::function_ref<Sig> rf{fn};
    check(rf, 52);
  }
  {  // three levels
    std::move_only_function<Sig> f{std::copyable_function<Sig>{std::function<Sig>{by_value}}};
    check(f, 60);
    std::function_ref<Sig> r{f};
    check(r, 61);
  }
  return 0;
}
