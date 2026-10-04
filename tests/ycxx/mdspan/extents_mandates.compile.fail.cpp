// [mdspan.extents.overview]/1: "Mandates: ... each element of Extents is either equal to
// dynamic_extent, or is representable as a value of type IndexType." 300 does not fit in
// signed char.
#include <mdspan>

std::extents<signed char, 2, 300> e;
