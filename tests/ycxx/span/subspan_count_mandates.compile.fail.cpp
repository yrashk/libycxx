// [span.sub]/7: subspan<Offset, Count>(): "Mandates: Offset <= Extent && (Count ==
// dynamic_extent || Count <= Extent - Offset) is true." Here Count > Extent - Offset.
#include <span>

void f(std::span<int, 4> s) { (void)s.subspan<1, 4>(); }
