// [optional.optional.general]/2: remove_cvref_t<X> must not be nullopt_t -- this applies to
// lvalue references too.
#include <optional>

std::optional<std::nullopt_t&> o;
