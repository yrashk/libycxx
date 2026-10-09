// EXPECT-ERROR: error: static assertion failed[^\n]*std::expected: E must be a valid argument for unexpected
// [expected.object.general]/2, [expected.void.general]/2: "A program that instantiates the
// definition of the template expected<T, E> with a type for the E parameter that is not a valid
// template argument for unexpected is ill-formed." ([expected.un.general]/2: non-object, array,
// specialization of unexpected, or cv-qualified types are not.) Here E is an array type.
#include <expected>

int n = sizeof(std::expected<int, int[2]>);
