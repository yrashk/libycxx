// [func.wrap.ref.ctor]/13-16: function_ref(constant_wrapper<c, F> f, U&& obj): calls are
// invoke_r<R>(f.value, static_cast<cv T&>(obj), call-args...); /17-20: the cv T* form calls
// invoke_r<R>(f.value, obj, call-args...). [dcl.fct]/6: taking the address of an explicit
// object member function gives an ordinary pointer to function, so the bound object becomes
// its first argument. [func.wrap.ref.deduct]/5: for F of the form R(*)(G, A...) noexcept(E),
// function_ref(constant_wrapper<c, F>, T&&) deduces function_ref<R(A...) noexcept(E)>.
// XFAIL-COMPILER: clang  clang rejects an explicit-object member function address as a template argument ("must explicitly qualify name of member function")
#include <functional>
#include <type_traits>
#include "check.hpp"

struct Self {
  int v;
  int twice(this const Self& s) { return 2 * s.v; }
  int add(this Self& s, int k) { return s.v += k; }
  long nx(this const Self& s) noexcept { return s.v; }
};

int main() {
  Self s{4};
  std::function_ref<int()> t1(std::cw<&Self::twice>, s);
  CHECK(t1() == 8);
  std::function_ref<int(int)> t2(std::cw<&Self::add>, s);
  t2(3);
  CHECK(s.v == 7 && t1() == 14);
  // the pointer form passes the pointer itself, which a Self& parameter does not accept
  static_assert(!std::is_constructible_v<std::function_ref<int(int)>,
                                         decltype(std::cw<&Self::add>), Self*>);
  t2(1);
  CHECK(s.v == 8);
  const Self cs{5};
  std::function_ref<int() const> t3(std::cw<&Self::twice>, cs);
  CHECK(t3() == 10);
  static_assert(!std::is_constructible_v<std::function_ref<int(int)>,
                                         decltype(std::cw<&Self::add>), const Self&>);
  std::function_ref d(std::cw<&Self::nx>, s);
  static_assert(std::is_same_v<decltype(d), std::function_ref<long() noexcept>>);
  CHECK(d() == 8L);
  std::function_ref d2(std::cw<&Self::add>, s);
  static_assert(std::is_same_v<decltype(d2), std::function_ref<int(int)>>);
  return 0;
}
