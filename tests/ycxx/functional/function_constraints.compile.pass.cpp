// [func.wrap.func.con]/9: template<class F> function(F&&) "Constraints:
// is_same_v<remove_cvref_t<F>, function> is false, and is_invocable_r_v<R, FD&, ArgTypes...>
// is true." /26: operator=(F&&) "Constraints: is_invocable_r_v<R, decay_t<F>&, ArgTypes...>
// is true." /29: operator=(reference_wrapper<F>) noexcept.
#include <functional>
#include <type_traits>

struct LvalueOnly {
  int operator()() & { return 0; }
  int operator()() && = delete;
};
struct RvalueOnly {
  int operator()() && { return 0; }
};
struct NonConstCall {
  int operator()() { return 0; }
};
struct ReturnsVoid {
  void operator()() {}
};
struct Base {};
struct Derived : Base {};
struct MakesDerived {
  Derived* operator()() const { return nullptr; }
};
struct S {
  int f(int) { return 0; }
  int v;
};

using F = std::function<int()>;
// the target is invoked as an lvalue (FD&)
static_assert(std::is_constructible_v<F, LvalueOnly>);
static_assert(!std::is_constructible_v<F, RvalueOnly>);
// non-const operator() is fine: FD& is non-const
static_assert(std::is_constructible_v<F, NonConstCall>);
static_assert(std::is_constructible_v<F, const NonConstCall&>);  // FD is NonConstCall
// result type must be implicitly convertible to R (INVOKE<R>)
static_assert(!std::is_constructible_v<F, ReturnsVoid>);
static_assert(std::is_constructible_v<std::function<void()>, ReturnsVoid>);
static_assert(std::is_constructible_v<std::function<void()>, LvalueOnly>);
static_assert(std::is_constructible_v<std::function<Base*()>, MakesDerived>);
static_assert(!std::is_constructible_v<std::function<Derived*()>, std::function<Base*()>>);
static_assert(std::is_constructible_v<std::function<Base*()>, std::function<Derived*()>>);
// argument types must be acceptable
static_assert(!std::is_constructible_v<std::function<int(int)>, F>);
static_assert(!std::is_constructible_v<F, int>);
static_assert(!std::is_constructible_v<F, int*>);
// member pointers go through INVOKE
static_assert(std::is_constructible_v<std::function<int(S&, int)>, decltype(&S::f)>);
static_assert(std::is_constructible_v<std::function<int(S*, int)>, decltype(&S::f)>);
static_assert(!std::is_constructible_v<std::function<int(const S&, int)>, decltype(&S::f)>);
static_assert(std::is_constructible_v<std::function<int&(S&)>, decltype(&S::v)>);
static_assert(!std::is_constructible_v<std::function<int&(const S&)>, decltype(&S::v)>);
// the converting constructor is implicit
static_assert(std::is_convertible_v<LvalueOnly, F>);
static_assert(std::is_convertible_v<int (*)(), F>);
// assignment has the same constraint
static_assert(std::is_assignable_v<F&, LvalueOnly>);
static_assert(!std::is_assignable_v<F&, RvalueOnly>);
static_assert(!std::is_assignable_v<F&, ReturnsVoid>);
static_assert(!std::is_assignable_v<F&, int>);
// reference_wrapper assignment is noexcept
static_assert(std::is_nothrow_assignable_v<F&, std::reference_wrapper<NonConstCall>>);
static_assert(std::is_assignable_v<F&, std::reference_wrapper<LvalueOnly>>);
