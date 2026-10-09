// EXPECT-ERROR: error: static assertion failed[^\n]*std::span::last: Count > Extent
// [span.sub]/4: last<Count>(): "Mandates: Count <= Extent is true."
#include <span>

void f(std::span<int, 2> s) { (void)s.last<3>(); }
