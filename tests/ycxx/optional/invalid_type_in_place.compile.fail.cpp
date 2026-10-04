// [optional.optional.general]/2: in_place_t is not a valid contained type.
#include <optional>

std::optional<std::in_place_t> o;
