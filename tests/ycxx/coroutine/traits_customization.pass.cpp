// [coroutine.traits.primary]/1-2: program-defined specializations of coroutine_traits supply the
// promise type ("shall define a publicly accessible nested type named promise_type").
// [dcl.fct.def.coroutine]/3: "The promise type of a coroutine is std::coroutine_traits<R, P1, ...,
// Pn>::promise_type, where R is the return type of the function, and P1...Pn is the sequence of
// types of the non-object function parameters, preceded by the type of the object parameter
// ([dcl.fct]) if the coroutine is a non-static member function." [dcl.fct]/5 (over.match.funcs):
// the object parameter of an implicit object member function is "lvalue reference to cv X" without
// a ref-qualifier or with &, "rvalue reference to cv X" with &&; for an explicit object member
// function it is the declared parameter type. Parameter types are adjusted ([dcl.fct]/5) before
// forming the list (an array parameter has pointer type).
// (The rvalue-ref-qualified case is in traits_object_parameter_rvalue.pass.cpp.)
// [coroutine.trivial.awaitables]: suspend_always / suspend_never.
#include <coroutine>
#include "check.hpp"

int last = 0;   // which promise type was used

template <int N> struct Promise {
  int get_return_object() { last = N; return N; }
  std::suspend_never initial_suspend() noexcept { return {}; }
  std::suspend_never final_suspend() noexcept { return {}; }
  void return_void() {}
  void unhandled_exception() {}
};

struct S;
struct Tag {};
template <> struct std::coroutine_traits<int> { using promise_type = Promise<1>; };
template <> struct std::coroutine_traits<int, Tag> { using promise_type = Promise<2>; };
template <> struct std::coroutine_traits<int, int*, long> { using promise_type = Promise<3>; };
template <> struct std::coroutine_traits<int, S&> { using promise_type = Promise<4>; };
template <> struct std::coroutine_traits<int, const S&> { using promise_type = Promise<5>; };
template <> struct std::coroutine_traits<int, const volatile S&> { using promise_type = Promise<7>; };
template <> struct std::coroutine_traits<int, S&, int> { using promise_type = Promise<8>; };
template <> struct std::coroutine_traits<int, S> { using promise_type = Promise<9>; };   // explicit object by value
template <class... A> struct std::coroutine_traits<long, A...> { using promise_type = Promise<10>; };   // partial

int free0() { co_return; }
int free_tag(Tag) { co_return; }
int free_adjusted(int a[4], long) { (void)a; co_return; }   // int[4] adjusted to int*
long any_args(double, char) { co_return; }

struct S {
  int m() { co_return; }
  int mc() const { co_return; }
  int mcv() const volatile & { co_return; }
  int mi(int) & { co_return; }
  int ex(this S) { co_return; }
  int exr(this const S&) { co_return; }
  static int st(Tag) { co_return; }   // static: no object parameter
};

int main() {
  CHECK(free0() == 1 && last == 1);
  CHECK(free_tag(Tag{}) == 2 && last == 2);
  int arr[4] = {};
  CHECK(free_adjusted(arr, 1L) == 3 && last == 3);
  CHECK(any_args(1.0, 'c') == 10 && last == 10);
  S s;
  CHECK(s.m() == 4 && last == 4);
  CHECK(s.mc() == 5 && last == 5);
  CHECK(s.mcv() == 7 && last == 7);
  CHECK(s.mi(0) == 8 && last == 8);
  CHECK(s.ex() == 9 && last == 9);
  CHECK(s.exr() == 5 && last == 5);
  CHECK(S::st(Tag{}) == 2 && last == 2);
  // trivial awaitables
  static_assert(std::suspend_always{}.await_ready() == false && std::suspend_never{}.await_ready() == true);
  return 0;
}
