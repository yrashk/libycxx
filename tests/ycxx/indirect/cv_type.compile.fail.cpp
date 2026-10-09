// EXPECT-ERROR: error: static assertion failed[^\n]*std::indirect: T must be a cv\-unqualified object type that is not an array, in_place_t or a specialization of in_place_type_t
// [indirect.general]/5: "A program that instantiates the definition of the template indirect<T,
// Allocator> with a type for the T parameter that is ... a cv-qualified type is ill-formed."
#include <memory>

std::indirect<const int> x;
