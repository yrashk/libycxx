// [const.wrap.class]: template<auto X, class T = decltype(X)> struct constant_wrapper :
// cw-operators { static constexpr decltype(auto) value = (X); using type = constant_wrapper;
// using value_type = decltype(X); constexpr operator decltype(value)() const noexcept; ... };
// [utility.syn]: template<auto X> constexpr auto cw = constant_wrapper<X>{};
// constexpr-param<T> is requires { typename constant_wrapper<T::value>; }.
#include <utility>
#include <concepts>
#include <type_traits>

using C5 = std::constant_wrapper<5>;
static_assert(C5::value == 5);
static_assert(std::is_same_v<decltype(C5::value), const int>);
static_assert(std::is_same_v<C5::value_type, int>);
static_assert(std::is_same_v<C5::type, C5>);
static_assert(std::is_same_v<decltype(std::cw<5>), const C5>);
static_assert(std::is_same_v<std::constant_wrapper<5, int>, C5>);  // default second argument
static_assert(std::is_empty_v<C5>);
static_assert(std::is_trivially_copyable_v<C5>);
static_assert(std::is_nothrow_default_constructible_v<C5>);
static_assert(std::is_same_v<std::constant_wrapper<'a'>::value_type, char>);
static_assert(std::is_same_v<std::constant_wrapper<5u>::value_type, unsigned>);
static_assert(!std::is_same_v<std::constant_wrapper<5>, std::constant_wrapper<5L>>);

// implicit conversion to the value
constexpr int as_int = std::cw<42>;
static_assert(as_int == 42);
static_assert(std::is_convertible_v<C5, int>);
static_assert(noexcept(static_cast<int>(std::cw<5>)));
constexpr int take(int x) { return x; }
static_assert(take(std::cw<9>) == 9);

// usable in constant expressions even when passed as a function parameter
constexpr auto pass_through(auto c) {
  static_assert(c == 3);  // the parameter's value is still a constant expression
  return c;
}
static_assert(pass_through(std::cw<3>) == 3);
template <class T>
constexpr bool size_is_four(T) {
  return T::value == 4;
}
static_assert(size_is_four(std::cw<4>));

// floating-point and class-type values
static_assert(std::cw<1.5>.value == 1.5);
static_assert(std::is_same_v<std::constant_wrapper<1.5>::value_type, double>);
struct Pt {
  int x, y;
};
static_assert(std::cw<Pt{1, 2}>.value.y == 2);
static_assert(std::is_same_v<std::constant_wrapper<Pt{1, 2}>::value_type, Pt>);
// X names a template parameter object (an lvalue), so value is declared as const Pt&
static_assert(std::is_same_v<decltype(std::constant_wrapper<Pt{1, 2}>::value), const Pt&>);
constexpr Pt pt = std::cw<Pt{3, 4}>;
static_assert(pt.x == 3);

// pointers and references to static storage
constexpr int global = 11;
static_assert(*std::cw<&global>.value == 11);
static_assert(std::is_same_v<std::constant_wrapper<&global>::value_type, const int*>);
