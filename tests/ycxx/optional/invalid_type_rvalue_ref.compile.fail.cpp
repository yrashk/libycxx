// [optional.optional.general]/2: a valid contained type is "an lvalue reference type or a
// complete non-array object type"; an rvalue reference is neither.
#include <optional>

std::optional<int&&> o;
