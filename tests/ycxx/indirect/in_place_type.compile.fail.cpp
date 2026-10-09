// EXPECT-ERROR: error: static assertion failed[^\n]*std::indirect: T must be a cv\-unqualified object type that is not an array, in_place_t or a specialization of in_place_type_t
// [indirect.general]/5: instantiating indirect with T = in_place_t is ill-formed.
#include <memory>
#include <utility>

std::indirect<std::in_place_t> x;
