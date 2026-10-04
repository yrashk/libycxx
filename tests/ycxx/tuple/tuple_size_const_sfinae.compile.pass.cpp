// [tuple.helper]/4-5: tuple_size<const T>: "Let TS denote tuple_size<T> of the cv-unqualified
// type T. If the expression TS::value is well-formed when treated as an unevaluated operand, then
// each specialization of the template meets the Cpp17UnaryTypeTrait requirements with a base
// characteristic of integral_constant<size_t, TS::value>. Otherwise, it has no member value.
// Access checking is performed as if in a context unrelated to TS and T. Only the validity of the
// immediate context of the expression is considered."
// /7-8: tuple_element<I, const T>: "Let TE denote tuple_element_t<I, T> of the cv-unqualified
// type T. Then each specialization of the template meets the Cpp17TransformationTrait
// requirements with a member typedef type that names the type add_const_t<TE>."
// (A program-defined tuple_size<X> must itself meet [tuple.helper]/1 ([namespace.std]/2), so only
// T without a tuple_size specialization exercises the "no member value" case.)
// [tuple.helper]/1: the tuple specializations have base integral_constant<size_t, N>.
#include <tuple>
#include <utility>
#include <array>
#include <cstddef>
#include <type_traits>

struct NotTupleLike {};
struct Custom {};
template <> struct std::tuple_size<Custom> : std::integral_constant<std::size_t, 2> {};
template <> struct std::tuple_element<0, Custom> { using type = int&; };
template <> struct std::tuple_element<1, Custom> { using type = const long; };

template <class T> concept HasValue = requires { std::tuple_size<T>::value; };
template <class T> concept HasType = requires { typename T::type; };

static_assert(!HasValue<const NotTupleLike>);
static_assert(!HasValue<const int>);
static_assert(!HasValue<const void>);
static_assert(!HasValue<const int[3]>);   // built-in arrays have no tuple_size
static_assert(HasValue<const Custom> && std::is_same_v<decltype(std::tuple_size<const Custom>::value), const std::size_t>);   // built-in arrays are not tuple-like for tuple_size
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 2>, std::tuple_size<const Custom>>);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 0>, std::tuple_size<const std::tuple<>>>);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 2>, std::tuple_size<const std::pair<int, int>>>);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 5>, std::tuple_size<const std::array<int, 5>>>);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 3>, std::tuple_size<std::tuple<int, int&, void*>>>);
static_assert(std::tuple_size<const std::tuple<int>>{}() == 1);
static_assert(std::tuple_size_v<const std::array<int, 0>> == 0);

// tuple_element<I, const T> = add_const_t<TE>
static_assert(std::is_same_v<std::tuple_element_t<0, const Custom>, int&>);          // const on a reference: no effect
static_assert(std::is_same_v<std::tuple_element_t<1, const Custom>, const long>);
static_assert(std::is_same_v<std::tuple_element_t<0, const std::tuple<int&>>, int&>);
static_assert(std::is_same_v<std::tuple_element_t<0, const std::tuple<int&&>>, int&&>);
static_assert(std::is_same_v<std::tuple_element_t<0, const std::tuple<int[2]>>, const int[2]>);
static_assert(std::is_same_v<std::tuple_element_t<0, const std::tuple<void (*)()>>, void (* const)()>);
static_assert(std::is_same_v<std::tuple_element_t<1, const std::pair<int, const char*>>, const char* const>);
static_assert(std::is_same_v<std::tuple_element_t<2, const std::array<short, 3>>, const short>);
static_assert(std::is_same_v<std::tuple_element<0, const std::tuple<int>>::type, const int>);

int main() {}
