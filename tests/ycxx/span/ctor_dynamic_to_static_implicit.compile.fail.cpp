// [span.cons]/26: the converting constructor is explicit when
// "extent != dynamic_extent && OtherExtent == dynamic_extent".
#include <span>

void f(std::span<int> d) { std::span<int, 2> s = d; (void)s; }
