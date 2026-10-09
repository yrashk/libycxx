// EXPECT-ERROR: error: static assertion failed[^\n]*std::unexpected: E must be a non\-array, non\-cv object type that is not a specialization of unexpected
// [expected.un.general]/2: "A program that instantiates the definition of unexpected for a
// non-object type, an array type, a specialization of unexpected, or a cv-qualified type is
// ill-formed." Here: a non-object type.
#include <expected>

int n = sizeof(std::unexpected<void>);
