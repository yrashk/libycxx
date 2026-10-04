// [func.wrap.move.inv]/3: move_only_function<R(Args...) cv ref noexcept(noex)>::operator()
// is "Equivalent to: return INVOKE<R>(static_cast<F inv-quals>(f), std::forward<ArgTypes>(
// args)...);" where inv-quals is cv ref if ref is non-empty, otherwise cv& ([func.wrap.move.
// class]/1). [func.wrap.copy.inv]/3: the same for copyable_function. So when one wrapper is
// the target of another ([func.wrap.general]/2), the outer call's cv/ref qualification reaches
// the inner wrapper and from there the innermost callable; noexcept signatures nest too.
#include <functional>
#include <utility>
#include "check.hpp"

struct Q {
  int operator()() & { return 1; }
  int operator()() const& { return 2; }
  int operator()() && { return 3; }
  int operator()() const&& { return 4; }
};
struct NX {
  int operator()(int x) const noexcept { return x + 100; }
};

int main() {
  {
    std::move_only_function<int()> f{std::copyable_function<int()>{Q{}}};
    CHECK(f() == 1);
    std::move_only_function<int() const> g{std::copyable_function<int() const>{Q{}}};
    CHECK(std::as_const(g)() == 2);
    std::move_only_function<int() &&> h{std::copyable_function<int() &&>{Q{}}};
    CHECK(std::move(h)() == 3);
    std::move_only_function<int() const&&> k{std::copyable_function<int() const&&>{Q{}}};
    CHECK(std::move(std::as_const(k))() == 4);
    std::move_only_function<int() const&> l{std::copyable_function<int() const&>{Q{}}};
    CHECK(std::as_const(l)() == 2);
    CHECK(std::move(std::as_const(l))() == 2);  // const& binds rvalues; target still called as const&
  }
  {
    // an unqualified outer wrapper calls a const inner one as an lvalue (inv-quals is &)
    std::move_only_function<int()> f{std::copyable_function<int() const>{Q{}}};
    CHECK(f() == 2);
    std::copyable_function<int() &&> h{std::copyable_function<int() &&>{Q{}}};
    CHECK(std::move(h)() == 3);
    // the inner wrapper is called as an rvalue, but being unqualified it calls its own target
    // as an lvalue (its inv-quals is &)
    std::copyable_function<int() &&> h2{std::copyable_function<int()>{Q{}}};
    CHECK(std::move(h2)() == 1);
  }
  {
    std::move_only_function<int(int) noexcept> f{std::copyable_function<int(int) const noexcept>{NX{}}};
    CHECK(f(1) == 101);
    static_assert(noexcept(f(1)));
    std::copyable_function<int(int) noexcept> g{std::copyable_function<int(int) noexcept>{NX{}}};
    CHECK(g(2) == 102);
    std::move_only_function<int(int)> h{std::move_only_function<int(int) const noexcept>{NX{}}};
    CHECK(h(3) == 103);
    std::function_ref<int(int) noexcept> r{f};
    CHECK(r(4) == 104);
  }
  return 0;
}
