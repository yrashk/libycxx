// [expected.un.general]/2: "A program that instantiates the definition of unexpected for a
// non-object type, an array type, a specialization of unexpected, or a cv-qualified type is
// ill-formed." Here: an array type.
#include <expected>

int n = sizeof(std::unexpected<int[2]>);
