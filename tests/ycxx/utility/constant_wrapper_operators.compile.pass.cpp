// [const.wrap.class]: cw-operators provides unary, binary, comparison and pseudo-mutator
// operators on constexpr-param arguments, each returning constant_wrapper<(expr on ::value)>,
// all noexcept; operator, is deleted; && and || are only provided when an operand's value is
// not convertible to bool.
#include <utility>
#include <compare>
#include <type_traits>

template <auto V>
using CW = std::constant_wrapper<V>;
template <class T, auto V>
constexpr bool is_cw = std::is_same_v<std::remove_cvref_t<T>, CW<V>>;

// unary
static_assert(is_cw<decltype(+std::cw<3>), 3>);
static_assert(is_cw<decltype(-std::cw<3>), -3>);
static_assert(is_cw<decltype(~std::cw<0>), -1>);
static_assert(is_cw<decltype(!std::cw<0>), true>);
static_assert(noexcept(-std::cw<3>));
// binary arithmetic and bitwise
static_assert(is_cw<decltype(std::cw<7> + std::cw<5>), 12>);
static_assert(is_cw<decltype(std::cw<7> - std::cw<5>), 2>);
static_assert(is_cw<decltype(std::cw<7> * std::cw<5>), 35>);
static_assert(is_cw<decltype(std::cw<7> / std::cw<2>), 3>);
static_assert(is_cw<decltype(std::cw<7> % std::cw<5>), 2>);
static_assert(is_cw<decltype(std::cw<1> << std::cw<4>), 16>);
static_assert(is_cw<decltype(std::cw<16> >> std::cw<2>), 4>);
static_assert(is_cw<decltype(std::cw<6> & std::cw<3>), 2>);
static_assert(is_cw<decltype(std::cw<6> | std::cw<3>), 7>);
static_assert(is_cw<decltype(std::cw<6> ^ std::cw<3>), 5>);
static_assert(is_cw<decltype(std::cw<2L> + std::cw<3>), 5L>);  // usual arithmetic conversions
static_assert(noexcept(std::cw<1> + std::cw<2>));
// comparisons
static_assert(is_cw<decltype(std::cw<1> < std::cw<2>), true>);
static_assert(is_cw<decltype(std::cw<1> <= std::cw<1>), true>);
static_assert(is_cw<decltype(std::cw<1> > std::cw<2>), false>);
static_assert(is_cw<decltype(std::cw<1> >= std::cw<2>), false>);
static_assert(is_cw<decltype(std::cw<1> == std::cw<1>), true>);
static_assert(is_cw<decltype(std::cw<1> != std::cw<1>), false>);
static_assert((std::cw<1> <=> std::cw<2>) < 0);  // the result type need not be structural
// mixing a constant_wrapper with a plain value falls back to the converted value
constexpr int five = 5;
static_assert(std::cw<2> + five == 7);
static_assert(std::is_same_v<decltype(std::cw<2> + five), int>);
// the second operand being a constexpr-param of another kind
struct HasValue {
  static constexpr int value = 10;
};
static_assert(is_cw<decltype(std::cw<1> + HasValue{}), 11>);

// && and || on bool-convertible values: no cw-operators overload; built-in operators apply
static_assert(std::is_same_v<decltype(std::cw<true> && std::cw<false>), bool>);
static_assert((std::cw<true> || std::cw<false>) == true);

// pseudo-mutators on a wrapper of a value that has suitable operators
struct Counter {
  int n;
  constexpr Counter operator++() const { return {n + 1}; }
  constexpr Counter operator++(int) const { return {n}; }
  constexpr Counter operator--() const { return {n - 1}; }
  constexpr Counter operator+=(int k) const { return {n + k}; }
  constexpr Counter operator=(int k) const { return {k}; }
};
constexpr auto cc = std::cw<Counter{4}>;
static_assert(decltype(++cc)::value.n == 5);
static_assert(decltype(cc++)::value.n == 4);
static_assert(decltype(--cc)::value.n == 3);
static_assert(decltype(cc += std::cw<3>)::value.n == 7);
static_assert(decltype(cc = std::cw<9>)::value.n == 9);
static_assert(noexcept(++cc));

// unary & and * on pointers
constexpr int arr[3] = {1, 2, 3};
static_assert(*std::cw<&arr[1]> == 2);
static_assert(decltype(*std::cw<&arr[2]>)::value == 3);
static_assert(*decltype(&std::cw<arr[0]>)::value == 1);  // unary & on a constexpr-param
