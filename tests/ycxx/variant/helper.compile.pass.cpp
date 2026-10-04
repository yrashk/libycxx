// [variant.helper]: variant_size / variant_size_v (incl. const T, with base characteristic
// integral_constant<size_t, N>), variant_alternative / variant_alternative_t (const T adds
// const to the alternative). [variant.syn]: variant_npos, variant_size_v is size_t.
// Note: volatile specializations were removed in C++26 and are not tested.
#include <variant>
#include <cstddef>
#include <type_traits>

using V = std::variant<int, const long, double&&(*)()>;

static_assert(std::variant_size<V>::value == 3);
static_assert(std::variant_size_v<V> == 3);
static_assert(std::variant_size_v<const V> == 3);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 3>, std::variant_size<V>>);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 3>, std::variant_size<const V>>);
static_assert(std::is_same_v<decltype(std::variant_size_v<V>), const std::size_t>);
static_assert(std::variant_size_v<std::variant<int>> == 1);

static_assert(std::is_same_v<std::variant_alternative_t<0, V>, int>);
static_assert(std::is_same_v<std::variant_alternative_t<1, V>, const long>);
static_assert(std::is_same_v<std::variant_alternative_t<2, V>, double&&(*)()>);
static_assert(std::is_same_v<std::variant_alternative<0, V>::type, int>);
static_assert(std::is_same_v<std::variant_alternative_t<0, const V>, const int>);
static_assert(std::is_same_v<std::variant_alternative_t<1, const V>, const long>);
static_assert(std::is_same_v<std::variant_alternative<0, const V>::type, const int>);
static_assert(std::is_same_v<std::variant_alternative_t<2, const V>, double&&(* const)()>);

// variant_size<T> for non-variant T is not defined (incomplete); SFINAE-friendly detection.
template <class T> concept has_size = requires { std::variant_size<T>::value; };
static_assert(!has_size<int>);
static_assert(has_size<V>);

static_assert(std::variant_npos == static_cast<std::size_t>(-1));
static_assert(std::is_same_v<decltype(std::variant_npos), const std::size_t>);
