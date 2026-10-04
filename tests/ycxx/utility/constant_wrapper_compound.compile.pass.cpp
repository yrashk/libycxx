// [const.wrap.class]: cw-operators provides the pseudo-mutators operator++/--(prefix and
// postfix), +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>= as explicit-object members returning
// constant_wrapper<(T::value op R::value)>, all noexcept, plus operator->*(L, R) returning
// constant_wrapper<L::value->*(R::value)>, and operator&& / operator|| constrained on an
// operand's value not being convertible to bool. A pseudo-mutator whose expression is invalid
// (e.g. on a const int value) removes the overload.
#include <utility>
#include <type_traits>

template <auto V>
using CW = std::constant_wrapper<V>;
template <class T, auto V>
constexpr bool is_cw = std::is_same_v<std::remove_cvref_t<T>, CW<V>>;

struct Num {
  int n;
  constexpr Num operator--(int) const { return {n * 100}; }
  constexpr Num operator-=(int k) const { return {n - k}; }
  constexpr Num operator*=(int k) const { return {n * k}; }
  constexpr Num operator/=(int k) const { return {n / k}; }
  constexpr Num operator%=(int k) const { return {n % k}; }
  constexpr Num operator&=(int k) const { return {n & k}; }
  constexpr Num operator|=(int k) const { return {n | k}; }
  constexpr Num operator^=(int k) const { return {n ^ k}; }
  constexpr Num operator<<=(int k) const { return {n << k}; }
  constexpr Num operator>>=(int k) const { return {n >> k}; }
};
constexpr auto c = std::cw<Num{12}>;
static_assert(decltype(c--)::value.n == 1200);
static_assert(decltype(c -= std::cw<2>)::value.n == 10);
static_assert(decltype(c *= std::cw<2>)::value.n == 24);
static_assert(decltype(c /= std::cw<5>)::value.n == 2);
static_assert(decltype(c %= std::cw<5>)::value.n == 2);
static_assert(decltype(c &= std::cw<4>)::value.n == 4);
static_assert(decltype(c |= std::cw<1>)::value.n == 13);
static_assert(decltype(c ^= std::cw<8>)::value.n == 4);
static_assert(decltype(c <<= std::cw<1>)::value.n == 24);
static_assert(decltype(c >>= std::cw<2>)::value.n == 3);
static_assert(noexcept(c -= std::cw<2>) && noexcept(c--));
// the right operand may be any constexpr-param
struct Two {
  static constexpr int value = 2;
};
static_assert(decltype(c -= Two{})::value.n == 10);

// pseudo-mutators are unavailable when the expression on the value is invalid
template <class T>
concept can_inc = requires(T t) { ++t; };
template <class T>
concept can_add_assign = requires(T t) { t += std::cw<1>; };
template <class T>
concept can_assign = requires(T t) { t = std::cw<1>; };
static_assert(!can_inc<CW<1>>);
static_assert(!can_add_assign<CW<1>>);
static_assert(!can_assign<CW<2>>);
static_assert(!can_inc<CW<Num{1}>>);  // Num has no prefix ++

// ->* on a pointer and a pointer to member
struct S {
  int v;
  int w;
};
constexpr S s_obj{8, 9};
static_assert(is_cw<decltype(std::cw<&s_obj>->*std::cw<&S::w>), 9>);
static_assert(noexcept(std::cw<&s_obj>->*std::cw<&S::v>));

// && and || for values not convertible to bool
struct Flag {
  bool b;
  constexpr Flag operator&&(Flag o) const { return {b && o.b}; }
  constexpr Flag operator||(Flag o) const { return {b || o.b}; }
};
static_assert(is_cw<decltype(std::cw<Flag{true}> && std::cw<Flag{false}>), Flag{false}>);
static_assert(is_cw<decltype(std::cw<Flag{true}> || std::cw<Flag{false}>), Flag{true}>);
static_assert(noexcept(std::cw<Flag{true}> && std::cw<Flag{true}>));
// one operand convertible to bool is not enough to exclude the overload
struct Gate {
  bool b;
  constexpr bool operator&&(bool o) const { return b && o; }
};
static_assert(is_cw<decltype(std::cw<Gate{true}> && std::cw<true>), true>);
static_assert(is_cw<decltype(std::cw<Gate{false}> && std::cw<true>), false>);
