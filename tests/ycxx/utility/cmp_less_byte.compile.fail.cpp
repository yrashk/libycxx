// EXPECT-ERROR-GCC: error: no matching function for call to 'cmp_less\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'cmp_less'
// [utility.intcmp]/4: cmp_less "Mandates: Each of T and U is a signed or unsigned integer
// type"; Note 1: these templates "cannot be used to compare byte".
#include <cstddef>
#include <utility>

bool b = std::cmp_less(std::byte{1}, 2);
