// [optional.optional.general]/2: "If a specialization of optional is instantiated with a type T
// that is not a valid contained type for optional, the program is ill-formed."
// An array type is not a valid contained type.
#include <optional>

std::optional<int[3]> o;
