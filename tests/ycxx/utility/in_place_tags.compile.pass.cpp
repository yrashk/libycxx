// [utility.syn]: struct in_place_t { explicit in_place_t() = default; }; inline constexpr
// in_place_t in_place{}; in_place_type_t<T> / in_place_type<T>; in_place_index_t<I> /
// in_place_index<I>; [pair.piecewise]: struct piecewise_construct_t { explicit
// piecewise_construct_t() = default; }; inline constexpr piecewise_construct_t
// piecewise_construct{}; "an empty class type used as a unique type".
#include <utility>
#include <cstddef>
#include <type_traits>

static_assert(std::is_same_v<decltype(std::in_place), const std::in_place_t>);
static_assert(std::is_same_v<decltype(std::in_place_type<int>), const std::in_place_type_t<int>>);
static_assert(std::is_same_v<decltype(std::in_place_index<3>), const std::in_place_index_t<3>>);
static_assert(std::is_same_v<decltype(std::piecewise_construct), const std::piecewise_construct_t>);

static_assert(std::is_empty_v<std::in_place_t>);
static_assert(std::is_empty_v<std::in_place_type_t<int>>);
static_assert(std::is_empty_v<std::in_place_index_t<0>>);
static_assert(std::is_empty_v<std::piecewise_construct_t>);
static_assert(std::is_trivially_copyable_v<std::in_place_t>);

// explicit default constructors: default-constructible, but not from {}
template <class T>
concept copy_list_init_from_empty = requires(void (*f)(T)) { f({}); };
static_assert(std::is_default_constructible_v<std::in_place_t>);
static_assert(!copy_list_init_from_empty<std::in_place_t>);
static_assert(!copy_list_init_from_empty<std::in_place_type_t<int>>);
static_assert(!copy_list_init_from_empty<std::in_place_index_t<1>>);
static_assert(!copy_list_init_from_empty<std::piecewise_construct_t>);
static_assert(copy_list_init_from_empty<int>);  // sanity check of the concept

// in_place_type_t accepts any type, including references, void and incomplete types
struct Incomplete;
using A = std::in_place_type_t<Incomplete>;
using B = std::in_place_type_t<int&>;
using C = std::in_place_type_t<void>;
static_assert(!std::is_same_v<std::in_place_type_t<int>, std::in_place_type_t<long>>);
static_assert(!std::is_same_v<std::in_place_index_t<0>, std::in_place_index_t<1>>);
static_assert(std::is_same_v<decltype(std::in_place_index_t<std::size_t(-1)>{}), std::in_place_index_t<std::size_t(-1)>>);

// usable in constant expressions
constexpr std::in_place_t ip = std::in_place;
constexpr std::piecewise_construct_t pc = std::piecewise_construct;
