// EXPECT-ERROR: error: static assertion failed[^\n]*std::span::subspan: Offset/Count out of range for Extent
// [span.sub]/7: subspan<Offset, Count>(): "Mandates: Offset <= Extent && (Count ==
// dynamic_extent || Count <= Extent - Offset) is true." Here Count > Extent - Offset.
#include <span>

void f(std::span<int, 4> s) { (void)s.subspan<1, 4>(); }
