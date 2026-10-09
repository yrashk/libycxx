// EXPECT-ERROR: error: static assertion failed[^\n]*layout_left_padded::mapping: the source's static extent\(0\) is not the static padding stride
// [mdspan.layout.leftpad.cons]/8: constructing layout_left_padded<P>::mapping from a
// layout_left::mapping of rank > 1: "Mandates: (static-padding-stride == dynamic_extent) ||
// (OtherExtents::static_extent(0) == dynamic_extent) || (static-padding-stride ==
// OtherExtents::static_extent(0)) is true." With extents<int, 6, 3> and P = 4 the
// static padding stride is 8 != 6.
#include <mdspan>

using E = std::extents<int, 6, 3>;
std::layout_left::mapping<E> src;
std::layout_left_padded<4>::mapping<E> dst(src);
