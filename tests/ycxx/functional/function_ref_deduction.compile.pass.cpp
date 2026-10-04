// [func.wrap.ref.deduct]/1: function_ref(F*) -> function_ref<F> (is_function_v<F>).
// /2-4: function_ref(constant_wrapper<c, F0>) -> function_ref<remove_pointer_t<F0>> when that
// is a function type. /5-6: function_ref(constant_wrapper<c, F>, T&&) ->
// function_ref<R(A...) noexcept(E)> where F is R(G::*)(A...) cv &opt noexcept(E), or M G::*
// (R = invoke_result_t<F, T&>, A empty, E true), or R(*)(G, A...) noexcept(E).
#include <functional>
#include <type_traits>

int f(int, char);
long g() noexcept;
struct S {
  int v;
  double m(int) const;
  void n(long, long) & noexcept;
};
int first(S&, short);
int firstn(const S*, short) noexcept;

void test(S& s) {
  std::function_ref a(&f);
  static_assert(std::is_same_v<decltype(a), std::function_ref<int(int, char)>>);
  std::function_ref b(g);
  static_assert(std::is_same_v<decltype(b), std::function_ref<long() noexcept>>);
  std::function_ref c(std::cw<f>);
  static_assert(std::is_same_v<decltype(c), std::function_ref<int(int, char)>>);
  std::function_ref d(std::cw<&g>);
  static_assert(std::is_same_v<decltype(d), std::function_ref<long() noexcept>>);
  std::function_ref e(std::cw<&S::m>, s);
  static_assert(std::is_same_v<decltype(e), std::function_ref<double(int)>>);
  std::function_ref h(std::cw<&S::n>, s);
  static_assert(std::is_same_v<decltype(h), std::function_ref<void(long, long) noexcept>>);
  std::function_ref k(std::cw<&S::v>, s);
  static_assert(std::is_same_v<decltype(k), std::function_ref<int&() noexcept>>);
  const S& cs = s;
  std::function_ref k2(std::cw<&S::v>, cs);
  static_assert(std::is_same_v<decltype(k2), std::function_ref<const int&() noexcept>>);
  std::function_ref p(std::cw<first>, s);
  static_assert(std::is_same_v<decltype(p), std::function_ref<int(short)>>);
  std::function_ref q(std::cw<firstn>, &s);
  static_assert(std::is_same_v<decltype(q), std::function_ref<int(short) noexcept>>);
  std::function_ref copy = a;
  static_assert(std::is_same_v<decltype(copy), std::function_ref<int(int, char)>>);
}
