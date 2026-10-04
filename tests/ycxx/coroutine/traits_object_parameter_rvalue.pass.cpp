// XFAIL-COMPILER: gcc  GCC 16.2 uses S& as the object parameter type of a &&-qualified member coroutine
// [dcl.fct.def.coroutine]/3: the promise type is coroutine_traits<R, P1, ..., Pn>::promise_type with
// P1 "the type of the object parameter ([dcl.fct])" for a non-static member function;
// [over.match.funcs.general]/4: for an implicit object member function declared with the &&
// ref-qualifier the type of the object parameter is "rvalue reference to cv X".
#include <coroutine>
#include "check.hpp"

int last = 0;
template <int N> struct Promise {
  int get_return_object() { last = N; return N; }
  std::suspend_never initial_suspend() noexcept { return {}; }
  std::suspend_never final_suspend() noexcept { return {}; }
  void return_void() {}
  void unhandled_exception() {}
};
struct S;
template <> struct std::coroutine_traits<int, S&> { using promise_type = Promise<1>; };
template <> struct std::coroutine_traits<int, S&&> { using promise_type = Promise<2>; };
template <> struct std::coroutine_traits<int, const S&&> { using promise_type = Promise<3>; };
struct S {
  int lv() & { co_return; }
  int rv() && { co_return; }
  int crv() const && { co_return; }
};

int main() {
  S s;
  CHECK(s.lv() == 1 && last == 1);
  CHECK(S{}.rv() == 2 && last == 2);
  CHECK(static_cast<const S&&>(s).crv() == 3 && last == 3);
  return 0;
}
