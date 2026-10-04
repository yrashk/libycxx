// [func.wrap.ref.ctor]/2: is-convertible-from-specialization<F> requires the same R and Args.
// /8: function_ref(F&& f): "If is-convertible-from-specialization<remove_cv_t<T>> is false,
// initializes bound-entity with addressof(f), and thunk-ptr with the address of a function
// thunk such that thunk(bound-entity, call-args...) is expression-equivalent to
// invoke_r<R>(static_cast<cv T&>(f), call-args...)." So a function_ref of a *different*
// signature (or of a non-convertible specialization) is bound by reference, like any other
// callable object: rebinding it afterwards is observed through the outer function_ref.
// Also: constructing from a move_only_function / copyable_function / function lvalue refers to
// that wrapper object, so reassigning the wrapper is observed too.
#include <functional>
#include <type_traits>
#include "check.hpp"

int one() { return 1; }
int two() { return 2; }
int half(int x) { return x / 2; }
int triple(int x) { return x * 3; }

int main() {
  // different R: function_ref<long()> from function_ref<int()>
  std::function_ref<int()> inner(&one);
  std::function_ref<long()> outer(inner);
  CHECK(outer() == 1L);
  inner = &two;
  CHECK(outer() == 2L);  // refers to inner itself

  // different parameter types
  std::function_ref<int(int)> in2(&half);
  std::function_ref<int(short)> out2(in2);
  CHECK(out2(short(8)) == 4);
  in2 = &triple;
  CHECK(out2(short(6)) == 18);

  // non-noexcept -> noexcept is not convertible-from-specialization; but the source is not
  // nothrow-invocable either, so that construction is excluded altogether
  static_assert(!std::is_constructible_v<std::function_ref<int() noexcept>, std::function_ref<int()>&>);

  // owning wrappers are referenced, not copied
  std::move_only_function<int()> m = &one;
  std::function_ref<int()> rm(m);
  CHECK(rm() == 1);
  m = &two;
  CHECK(rm() == 2);
  std::copyable_function<int() const> c = &one;
  std::function_ref<int() const> rc(c);
  c = &two;
  CHECK(rc() == 2);
  std::function<int()> f = &one;
  std::function_ref<long()> rf(f);
  f = &two;
  CHECK(rf() == 2L);
  return 0;
}
