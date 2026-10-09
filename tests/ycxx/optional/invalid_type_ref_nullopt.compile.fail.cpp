// EXPECT-ERROR: error: static assertion failed[^\n]*std::optional<T\&>: remove_cvref_t<T> must not be in_place_t or nullopt_t
// [optional.optional.general]/2: remove_cvref_t<X> must not be nullopt_t -- this applies to
// lvalue references too.
#include <optional>

std::optional<std::nullopt_t&> o;
