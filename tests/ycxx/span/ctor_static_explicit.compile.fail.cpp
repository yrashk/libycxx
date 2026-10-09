// EXPECT-ERROR-GCC: error: converting to 'std::span<int, 3>'.*use explicit constructor
// EXPECT-ERROR-CLANG: error: chosen constructor is explicit in copy-initialization
// EXPECT-ERROR-CLANG: \bspan\(_It first, size_type
// [span.cons]/3: "template<class It> constexpr explicit(extent != dynamic_extent)
// span(It first, size_type count);" is explicit for a static extent, so copy-list-
// initialisation is ill-formed.
#include <span>

int a[3];
std::span<int, 3> s = {a + 0, 3};
