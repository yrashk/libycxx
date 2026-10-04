// [span.sub]/1: first<Count>(): "Mandates: Count <= Extent is true."
#include <span>

void f(std::span<int, 2> s) { (void)s.first<3>(); }
