// [mdspan.extents.overview]/1: "Mandates: IndexType is a signed or unsigned integer type".
// bool is not.
#include <mdspan>

std::extents<bool, 1> e;
