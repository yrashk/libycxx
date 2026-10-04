// [func.wrap.func.inv]/2, [func.wrap.move.inv]/3, [func.wrap.copy.inv]/3,
// [func.wrap.ref.inv]/2: every wrapper invokes its target with
// std::forward<ArgTypes>(args)..., so for reference parameter types the innermost callable
// binds to the caller's own object through any nesting of function, copyable_function,
// move_only_function and function_ref ([func.wrap.general]/2), and nothing is copied or
// moved.
#include <functional>
#include <utility>
#include "check.hpp"

struct Arg {
  static inline int copies = 0;
  static inline int moves = 0;
  int v = 0;
  Arg() = default;
  Arg(const Arg& o) : v(o.v) { ++copies; }
  Arg(Arg&& o) noexcept : v(o.v) { ++moves; }
};

const Arg* where = nullptr;
auto lref = [](Arg& a) { where = &a; ++a.v; };
auto cref = [](const Arg& a) { where = &a; };
auto rref = [](Arg&& a) { where = &a; Arg stolen = std::move(a); (void)stolen; };

template <class F, class Call>
void check(F& f, Call call) {
  Arg a;
  Arg::copies = Arg::moves = 0;
  where = nullptr;
  call(f, a);
  CHECK(where == &a);
  CHECK(Arg::copies == 0);
}

int main() {
  auto pass_l = [](auto& f, Arg& a) { f(a); };
  auto pass_r = [](auto& f, Arg& a) { f(std::move(a)); };
  {
    std::move_only_function<void(Arg&)> f{std::copyable_function<void(Arg&)>{lref}};
    check(f, pass_l);
    std::function<void(Arg&)> g{std::copyable_function<void(Arg&)>{lref}};
    check(g, pass_l);
    std::copyable_function<void(Arg&)> h{std::function<void(Arg&)>{lref}};
    check(h, pass_l);
    std::function_ref<void(Arg&)> r{f};
    check(r, pass_l);
    // the callee's modification is visible to the caller
    Arg a;
    f(a);
    r(a);
    CHECK(a.v == 2);
  }
  {
    std::move_only_function<void(const Arg&)> f{std::copyable_function<void(const Arg&)>{cref}};
    check(f, pass_l);
    std::function<void(const Arg&)> g{std::copyable_function<void(const Arg&)>{cref}};
    check(g, pass_l);
    std::function_ref<void(const Arg&)> r{g};
    check(r, pass_l);
  }
  {
    std::move_only_function<void(Arg&&)> f{std::copyable_function<void(Arg&&)>{rref}};
    check(f, pass_r);
    CHECK(Arg::moves == 1);  // only the callee's own move
    std::function<void(Arg&&)> g{std::copyable_function<void(Arg&&)>{rref}};
    check(g, pass_r);
    CHECK(Arg::moves == 1);
    std::function_ref<void(Arg&&)> r{f};
    check(r, pass_r);
    CHECK(Arg::moves == 1);
  }
  return 0;
}
