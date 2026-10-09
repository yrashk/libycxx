// EXPECT-ERROR: error: static assertion failed[^\n]*optional<T>: T must be a complete non\-array object type other than in_place_t/nullopt_t
// [optional.optional.general]/2: in_place_t is not a valid contained type.
#include <optional>

std::optional<std::in_place_t> o;
