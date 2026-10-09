// EXPECT-ERROR: error: static assertion failed[^\n]*std::extents: IndexType must be a signed or unsigned integer type
// [mdspan.extents.overview]/1: "Mandates: IndexType is a signed or unsigned integer type".
// bool is not.
#include <mdspan>

std::extents<bool, 1> e;
