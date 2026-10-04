// [indirect.general]/5: instantiating indirect with T = in_place_t is ill-formed.
#include <memory>
#include <utility>

std::indirect<std::in_place_t> x;
