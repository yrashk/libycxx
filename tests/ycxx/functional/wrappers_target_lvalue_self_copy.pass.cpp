// Which overload of the target's operator() a call reaches:
// [func.wrap.func.inv]/1: function::operator() const "Returns: INVOKE<R>(f, std::forward<
//   ArgTypes>(args)...) where f is the target object" -- an lvalue of the (non-const) target
//   type, although operator() itself is const.
// [func.wrap.ref.ctor]/8: function_ref(F&& f) calls "invoke_r<R>(static_cast<cv T&>(f),
//   call-args...)" -- always an lvalue, const only if the signature is const, even when f was
//   an rvalue or a const object (then T is const and cv T& is const).
// Self copy-assignment: [func.wrap.func.con]/19 "As if by function(f).swap(*this)";
//   [func.wrap.copy.ctor]/24 "Equivalent to: copyable_function(f).swap(*this)" -- the target
//   survives, for small and large targets alike. Self-swap leaves the target in place
//   ([func.wrap.func.mod]/2, [func.wrap.copy.util]/1, [func.wrap.move.util]/1).
// [func.wrap.ref.class]: function_ref is trivially copyable; self-assignment keeps the binding.
#include <functional>
#include <string>
#include <utility>
#include "check.hpp"

struct Q {
  int operator()() & { return 1; }
  int operator()() const& { return 2; }
  int operator()() && { return 3; }
  int operator()() const&& { return 4; }
};
struct BigQ : Q {
  char pad[256] = {};
};

template <class F>
void dispatch() {
  F q;
  std::function<int()> f = q;
  CHECK(f() == 1);
  CHECK(std::as_const(f)() == 1);
  CHECK(std::move(f)() == 1);
  std::function_ref<int()> r = q;
  CHECK(r() == 1);
  std::function_ref<int()> rr = std::move(q);  // still an lvalue call
  CHECK(rr() == 1);
  std::function_ref<int()> rc = std::as_const(q);  // T = const F
  CHECK(rc() == 2);
  std::function_ref<int() const> c = q;
  CHECK(c() == 2);
}

template <class W, class F>
void self_copy(int expected) {
  W w = F{};
  W& alias = w;
  w = alias;
  CHECK(w && w() == expected);
  w.swap(w);
  CHECK(w() == expected);
  swap(w, alias);
  CHECK(w() == expected);
}

int main() {
  dispatch<Q>();
  dispatch<BigQ>();
  self_copy<std::function<int()>, Q>(1);
  self_copy<std::function<int()>, BigQ>(1);
  self_copy<std::copyable_function<int()>, Q>(1);
  self_copy<std::copyable_function<int()>, BigQ>(1);
  self_copy<std::copyable_function<int() const>, BigQ>(2);
  {
    std::string s(100, 'x');
    std::function<std::size_t()> f = [s] { return s.size(); };
    auto& a = f;
    f = a;
    CHECK(f() == 100);
    std::copyable_function<std::size_t() const> g = [s] { return s.size(); };
    auto& b = g;
    g = b;
    CHECK(g() == 100);
  }
  {
    std::move_only_function<int()> m = BigQ{};
    m.swap(m);
    CHECK(m() == 1);
    swap(m, m);
    CHECK(m() == 1);
  }
  {
    Q q;
    std::function_ref<int()> r = q;
    auto& a = r;
    r = a;
    CHECK(r() == 1);
  }
  return 0;
}
