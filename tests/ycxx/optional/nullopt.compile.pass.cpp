// [optional.nullopt]: nullopt_t is an empty class with no default constructor and no
// initializer-list constructor, not an aggregate; models copyable and
// three_way_comparable<strong_ordering> (see nullopt_compare.compile.pass.cpp). nullopt is an inline constexpr nullopt_t.
// [optional.syn]: in_place_t is usable with optional.
#include <optional>
#include <compare>
#include <concepts>
#include <type_traits>

static_assert(std::is_empty_v<std::nullopt_t>);
static_assert(!std::is_default_constructible_v<std::nullopt_t>);
static_assert(!std::is_aggregate_v<std::nullopt_t>);
static_assert(std::copyable<std::nullopt_t>);
static_assert(std::is_same_v<decltype(std::nullopt), const std::nullopt_t>);
// not constructible from {} (would make `o = {}` ambiguous)
template <class T> concept from_braces = requires { T{}; };
static_assert(!from_braces<std::nullopt_t>);
constexpr std::nullopt_t copy = std::nullopt;
static_assert(std::is_trivially_copyable_v<std::nullopt_t>);
