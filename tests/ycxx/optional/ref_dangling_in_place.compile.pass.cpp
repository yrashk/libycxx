// [optional.ref.ctor]/1.2: the in_place constructor is constrained on
// reference_constructs_from_temporary_v<T&, Arg> being false.
// [optional.ref.ctor]/7: the U&& constructor is deleted (not merely unconstrained) for
// temporaries, so it still participates: is_constructible is false either way.
#include <optional>
#include <type_traits>

static_assert(!std::is_constructible_v<std::optional<const int&>, std::in_place_t, int>);
static_assert(!std::is_constructible_v<std::optional<const int&>, std::in_place_t, long&>);
static_assert(std::is_constructible_v<std::optional<const int&>, std::in_place_t, int&>);
static_assert(!std::is_constructible_v<std::optional<const int&>, int>);
static_assert(!std::is_constructible_v<std::optional<const int&>, const long&>);
static_assert(std::is_constructible_v<std::optional<const int&>, const int&>);
static_assert(!std::is_constructible_v<std::optional<const int&>, std::optional<int>>);  // optional<U>&&: deleted
static_assert(!std::is_constructible_v<std::optional<const int&>, const std::optional<long>&>);
