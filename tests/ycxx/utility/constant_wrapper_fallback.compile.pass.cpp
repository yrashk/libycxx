// [const.wrap.class]: each cw-operators overload is declared with a trailing return type
// constant_wrapper<(expr)>, so when expr is not a constant expression (or its type is not
// structural) substitution fails and the overload is not viable; the ordinary operators then
// apply to the values obtained through the conversion operator. /4: operator() uses
// constant_wrapper<INVOKE(...)>{} only if that "is a valid type", "otherwise ...
// INVOKE(value, std::forward<Args>(args)...)". The conversion operator returns
// decltype(value), which for a class type is a reference to the template parameter object.
// /3 Example 1: values computed from constant_wrappers stay usable in constant expressions
// after being passed through function parameters.
#include <utility>
#include <type_traits>

template <auto V>
using CW = std::constant_wrapper<V>;
template <class T, auto V>
constexpr bool is_cw = std::is_same_v<std::remove_cvref_t<T>, CW<V>>;

// division by zero: no constant result, so the built-in operator on int is used
static_assert(std::is_same_v<decltype(std::cw<1> / std::cw<0>), int>);
static_assert(std::is_same_v<decltype(std::cw<1> % std::cw<0>), int>);
static_assert(is_cw<decltype(std::cw<6> / std::cw<3>), 2>);

// a non-structural INVOKE result falls back to the plain call
class Priv {
  int v;

 public:
  constexpr explicit Priv(int x) : v(x) {}
  constexpr int get() const { return v; }
};
constexpr Priv make(int x) { return Priv(x); }
static_assert(std::is_same_v<decltype(std::cw<make>(std::cw<3>)), Priv>);
static_assert(std::cw<make>(std::cw<3>).get() == 3);

// the conversion yields the template parameter object itself for class types
struct Pt {
  int x, y;
};
static_assert(&static_cast<const Pt&>(std::cw<Pt{1, 2}>) == &std::constant_wrapper<Pt{1, 2}>::value);
static_assert(std::is_convertible_v<CW<Pt{1, 2}>, const Pt&>);
static_assert(noexcept(static_cast<const Pt&>(std::cw<Pt{1, 2}>)));

// [const.wrap.class] Example 1
constexpr auto initial_phase(auto q1, auto q2) { return q1 + q2; }
constexpr auto middle_phase(auto tbd) { return tbd; }
template <class G, class A>
constexpr bool final_phase(G gathered, A available) {
  if constexpr (gathered == available)
    return true;
  else
    return false;
}
constexpr bool planning() {
  auto gathered_quantity = middle_phase(initial_phase(std::cw<42>, std::cw<13>));
  static_assert(gathered_quantity == 55);
  auto all_available = std::cw<55>;
  return final_phase(gathered_quantity, all_available);
}
static_assert(planning());
static_assert(is_cw<decltype(initial_phase(std::cw<42>, std::cw<13>)), 55>);

// mixed operands: a run-time value makes the result a plain value
constexpr int seven = 7;
static_assert(std::is_same_v<decltype(std::cw<1> == seven), bool>);
static_assert(std::is_same_v<decltype(-std::cw<1> + seven), int>);
