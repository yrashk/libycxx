// [span.cons]/13: array constructor "Constraints: extent == dynamic_extent || N == extent".
// EXPECT-ERROR-GCC: no matching function for call to .std::span<int, 4>::span\(int \[3\]\)
// EXPECT-ERROR-CLANG: no matching constructor for initialization of 'std::span<int, 4>'
#include <span>

int a[3];
std::span<int, 4> s(a);
