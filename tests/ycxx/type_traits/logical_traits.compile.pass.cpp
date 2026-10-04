// [meta.logical]/3,8: conjunction/disjunction short-circuit: once some bool(Bi::value) decides
// the result, "instantiating ...::value does not require the instantiation of Bj::value for j>i"
// (so later arguments may even be incomplete types). /5, /10: the specialization "has a public and
// unambiguous base that is either the first type Bi in the list true_type, B1, ..., BN for which
// bool(Bi::value) is false [disjunction: false_type, ..., true], or if there is no such Bi, the
// last type in the list" (not necessarily true_type/false_type). /6, /11: "The member names of
// the base class, other than conjunction and operator=, shall not be hidden and shall be
// unambiguously available in conjunction." /12: negation<B> is a Cpp17UnaryTypeTrait with base
// characteristic bool_constant<!bool(B::value)>.
// [meta.help]: integral_constant<T, v>: value, value_type, type, constexpr operator value_type()
// and operator()(); bool_constant, true_type, false_type.
#include <type_traits>

struct Incomplete;
template <class T> struct Poison {   // instantiating Poison<T>::value is an error
  static constexpr bool value = T::no_such_member;
};

struct Rich {   // a trait-like type with extra members that must stay visible
  static constexpr bool value = true;
  using type = int;
  using value_type = char;
  static constexpr int extra = 7;
  template <class X> using tmpl = X*;
  constexpr int fn() const { return 42; }
};
struct RichFalse {
  static constexpr bool value = false;
  using type = long;
  static constexpr int extra = 9;
};
struct ConvBool {   // value of a class type implicitly convertible to bool
  struct V { constexpr operator bool() const { return false; } };
  static constexpr V value{};
};
using Two = std::integral_constant<int, 2>;
using Zero = std::integral_constant<int, 0>;
enum class SE : unsigned char { a = 3 };
using EnumC = std::integral_constant<SE, SE::a>;

// Base characteristic.
static_assert(std::is_base_of_v<std::true_type, std::conjunction<>>);
static_assert(std::is_base_of_v<std::false_type, std::disjunction<>>);
static_assert(std::is_base_of_v<Two, std::conjunction<Two>>);
static_assert(std::is_base_of_v<Zero, std::conjunction<Two, Zero, std::true_type>>);
static_assert(std::is_base_of_v<std::true_type, std::conjunction<Two, std::true_type>>);
static_assert(std::is_base_of_v<Two, std::disjunction<Zero, Two, std::true_type>>);
static_assert(std::is_base_of_v<Zero, std::disjunction<std::false_type, Zero>>);
static_assert(std::is_same_v<decltype(std::conjunction<Two>::value), const int>);
static_assert(std::conjunction<Two>::value == 2 && std::conjunction_v<Two> == true);
static_assert(std::conjunction<Two, Zero>::value == 0 && std::disjunction<Zero, Two>::value == 2);
static_assert(std::is_same_v<std::conjunction<Two, Zero>::type, Zero>);
static_assert(std::is_same_v<std::disjunction<Zero, Two>::value_type, int>);
static_assert(std::is_base_of_v<EnumC, std::conjunction<EnumC>>);
static_assert(std::conjunction<EnumC>::value == SE::a);
static_assert(!std::conjunction_v<std::true_type, ConvBool> && !std::disjunction_v<ConvBool>);
static_assert(std::is_base_of_v<ConvBool, std::conjunction<std::true_type, ConvBool, std::true_type>>);

// Member names of the base are not hidden.
static_assert(std::is_same_v<std::conjunction<Rich>::type, int>);
static_assert(std::is_same_v<std::conjunction<std::true_type, Rich>::type, int>);
static_assert(std::is_same_v<std::conjunction<std::true_type, Rich>::value_type, char>);
static_assert(std::conjunction<Rich, Rich>::extra == 7);
static_assert(std::is_same_v<std::conjunction<Rich>::tmpl<int>, int*>);
static_assert(std::conjunction<Rich>{}.fn() == 42);
static_assert(std::is_same_v<std::disjunction<std::false_type, Rich>::type, int>);
static_assert(std::is_same_v<std::disjunction<Rich, std::true_type>::value_type, char>);
static_assert(std::disjunction<RichFalse>::extra == 9 && std::is_same_v<std::disjunction<RichFalse>::type, long>);
static_assert(std::is_same_v<std::conjunction<Rich, RichFalse, Rich>::type, long>);
static_assert(std::disjunction<RichFalse, Rich>::extra == 7);

// Short-circuiting: later arguments are not inspected.
static_assert(!std::conjunction_v<std::false_type, Incomplete>);
static_assert(!std::conjunction_v<std::true_type, std::false_type, Poison<int>>);
static_assert(std::disjunction_v<std::true_type, Incomplete>);
static_assert(std::disjunction_v<std::false_type, Two, Poison<int>, Incomplete>);
static_assert(std::is_base_of_v<std::false_type, std::conjunction<std::false_type, Poison<int>>>);
static_assert(std::is_base_of_v<Two, std::disjunction<Two, Poison<long>>>);
template <class... B> constexpr bool conj_in_template = std::conjunction<B...>::value;
static_assert(!conj_in_template<Zero, Poison<char>>);

// negation
static_assert(std::is_base_of_v<std::false_type, std::negation<std::true_type>>);
static_assert(std::is_base_of_v<std::true_type, std::negation<std::false_type>>);
static_assert(std::is_base_of_v<std::false_type, std::negation<Two>>);
static_assert(std::is_base_of_v<std::true_type, std::negation<Zero>>);
static_assert(std::is_base_of_v<std::true_type, std::negation<ConvBool>>);
static_assert(std::is_same_v<std::negation<Two>::type, std::false_type>);
static_assert(std::is_same_v<decltype(std::negation_v<Two>), const bool> && !std::negation_v<Rich>);
static_assert(std::negation_v<std::negation<std::negation<std::false_type>>>);

// integral_constant ([meta.help])
using IC = std::integral_constant<long, -5>;
static_assert(IC::value == -5 && std::is_same_v<IC::value_type, long> && std::is_same_v<IC::type, IC>);
static_assert(IC{} == -5L && IC{}() == -5L);
static_assert(std::is_same_v<decltype(IC{}()), long> && noexcept(IC{}()) && noexcept(static_cast<long>(IC{})));
static_assert(std::is_nothrow_default_constructible_v<IC> && std::is_trivially_copyable_v<IC> && std::is_empty_v<IC>);
static_assert(std::is_same_v<std::bool_constant<true>, std::true_type>);
static_assert(std::is_same_v<std::true_type::type, std::true_type> && std::is_same_v<std::false_type::value_type, bool>);
static_assert(std::is_same_v<std::integral_constant<SE, SE::a>::value_type, SE>);
static_assert(+std::integral_constant<char, 'a'>{} == 97);   // implicit conversion then promotion
constexpr int arr[std::integral_constant<int, 3>{}] = {};    // usable as a constant expression
static_assert(sizeof(arr) == 3 * sizeof(int));
// integral_constant with a pointer or pointer-to-member value ([meta.help] places no
// restriction on T beyond being usable as a template parameter type).
constexpr int gi = 0;
using PC = std::integral_constant<const int*, &gi>;
static_assert(PC::value == &gi && PC{}() == &gi);

int main() {}
