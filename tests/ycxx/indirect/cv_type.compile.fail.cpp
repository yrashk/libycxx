// [indirect.general]/5: "A program that instantiates the definition of the template indirect<T,
// Allocator> with a type for the T parameter that is ... a cv-qualified type is ill-formed."
#include <memory>

std::indirect<const int> x;
