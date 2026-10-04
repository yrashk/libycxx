// FLAGS: -Wno-deprecated-declarations -Wno-deprecated
// [depr.tuple] (Annex D, normative): <tuple> additionally declares
//   template<class T> struct tuple_size<volatile T>;
//   template<class T> struct tuple_size<const volatile T>;
//   template<size_t I, class T> struct tuple_element<I, volatile T>;
//   template<size_t I, class T> struct tuple_element<I, const volatile T>;
// /2-3: with TS = tuple_size<T>, if TS::value is well-formed (access checked in an unrelated
// context, immediate context only) the base characteristic is integral_constant<size_t,
// TS::value>, otherwise there is no member value. /5: tuple_element::type is volatile TE /
// const volatile TE. [depr]/2: deprecated features are still part of the standard ("Normative").
#include <tuple>
#include <utility>
#include <array>
#include <cstddef>
#include <type_traits>

template <class T> concept HasValue = requires { std::tuple_size<T>::value; };

static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 2>, std::tuple_size<volatile std::tuple<int, long>>>);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 2>, std::tuple_size<const volatile std::pair<int, int>>>);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 3>, std::tuple_size<volatile std::array<char, 3>>>);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 0>, std::tuple_size<const volatile std::tuple<>>>);
static_assert(std::tuple_size_v<volatile std::tuple<int>> == 1);
static_assert(!HasValue<volatile int> && !HasValue<const volatile int>);
static_assert(std::is_same_v<std::tuple_element_t<0, volatile std::tuple<int, long>>, volatile int>);
static_assert(std::is_same_v<std::tuple_element_t<1, const volatile std::tuple<int, long>>, const volatile long>);
static_assert(std::is_same_v<std::tuple_element_t<0, volatile std::tuple<int&>>, int&>);
static_assert(std::is_same_v<std::tuple_element_t<1, volatile std::pair<int, char>>, volatile char>);
static_assert(std::is_same_v<std::tuple_element_t<2, const volatile std::array<short, 3>>, const volatile short>);

int main() {}
