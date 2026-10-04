// [const.wrap.class]/4-7: static operator()(Args&&... args): call-expr is
// constant_wrapper<INVOKE(value, remove_cvref_t<Args>::value...)>{} if all Args satisfy
// constexpr-param and that type is valid, otherwise INVOKE(value, std::forward<Args>(args)...);
// constrained on call-expr being valid; noexcept(call-expr). /8-11: operator[] likewise with
// value[remove_cvref_t<Args>::value...].
#include <utility>
#include <type_traits>

constexpr int twice(int x) { return 2 * x; }
int runtime_only(int x) { return x; }
constexpr int nothrow_fn(int x) noexcept { return x; }
struct S {
  int v;
  constexpr int get() const { return v; }
};
constexpr S s_obj{8};
constexpr int arr[4] = {10, 20, 30, 40};
struct Grid {
  constexpr int operator[](int i, int j) const { return i * 10 + j; }
};

template <auto V>
using CW = std::constant_wrapper<V>;
template <class T, auto V>
constexpr bool is_cw = std::is_same_v<std::remove_cvref_t<T>, CW<V>>;

// all arguments are constexpr-params: the result is a constant_wrapper
static_assert(is_cw<decltype(std::cw<twice>(std::cw<21>)), 42>);
static_assert(is_cw<decltype(std::cw<&S::get>(std::cw<&s_obj>)), 8>);
// otherwise the plain INVOKE result
int i = 3;
static_assert(std::is_same_v<decltype(std::cw<twice>(i)), int>);
static_assert(std::is_same_v<decltype(std::cw<runtime_only>(std::cw<1>)), int>);  // not a constant
// the call operator is static
static_assert(is_cw<decltype(CW<twice>::operator()(std::cw<1>)), 2>);
static_assert(CW<twice>::operator()(5) == 10);
// constraints and exception specification
static_assert(!std::is_invocable_v<CW<twice>, int*>);
static_assert(std::is_invocable_v<CW<twice>, int>);
static_assert(std::is_nothrow_invocable_v<CW<twice>, decltype(std::cw<1>)>);  // a constant_wrapper{}
static_assert(std::is_nothrow_invocable_v<CW<nothrow_fn>, int>);
static_assert(!std::is_nothrow_invocable_v<CW<twice>, int>);

// subscripts
static_assert(is_cw<decltype(std::cw<arr>[std::cw<2>]), 30>);
static_assert(std::is_same_v<decltype(std::cw<arr>[i]), const int&>);
static_assert(is_cw<decltype(std::cw<Grid{}>[std::cw<1>, std::cw<2>]), 12>);
static_assert(CW<Grid{}>::operator[](3, 4) == 34);
template <class C, class I>
concept subscriptable = requires(C c, I idx) { c[idx]; };
static_assert(subscriptable<CW<arr>, int>);
static_assert(!subscriptable<CW<arr>, int*>);
static_assert(!subscriptable<CW<5>, int>);
