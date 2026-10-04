// [span.cons]/3: "template<class It> constexpr explicit(extent != dynamic_extent)
// span(It first, size_type count);" is explicit for a static extent, so copy-list-
// initialisation is ill-formed.
#include <span>

int a[3];
std::span<int, 3> s = {a + 0, 3};
