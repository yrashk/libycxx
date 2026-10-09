// EXPECT-ERROR: error: static assertion failed[^\n]*optional<T>: T must be a complete non\-array object type other than in_place_t/nullopt_t
// [optional.optional.general]/2: a valid contained type is "an lvalue reference type or a
// complete non-array object type"; an rvalue reference is neither.
#include <optional>

std::optional<int&&> o;
