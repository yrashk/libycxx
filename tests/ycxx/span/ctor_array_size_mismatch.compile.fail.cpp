// [span.cons]/13: array constructor "Constraints: extent == dynamic_extent || N == extent".
#include <span>

int a[3];
std::span<int, 4> s(a);
