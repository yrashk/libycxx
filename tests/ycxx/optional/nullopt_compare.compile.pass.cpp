// [optional.nullopt]/2: "nullopt_t models copyable and three_way_comparable<strong_ordering>."
#include <optional>
#include <compare>
#include <concepts>

static_assert(std::three_way_comparable<std::nullopt_t, std::strong_ordering>);
static_assert(std::nullopt == std::nullopt);
static_assert(!(std::nullopt != std::nullopt));
static_assert((std::nullopt <=> std::nullopt) == std::strong_ordering::equal);
