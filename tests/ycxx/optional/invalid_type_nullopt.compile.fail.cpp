// EXPECT-ERROR: error: static assertion failed[^\n]*optional<T>: T must be a complete non\-array object type other than in_place_t/nullopt_t
// [optional.optional.general]/2: remove_cvref_t<X> must be a type other than in_place_t or
// nullopt_t for X to be a valid contained type; otherwise the program is ill-formed.
#include <optional>

std::optional<const std::nullopt_t> o;
