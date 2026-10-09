// EXPECT-ERROR-GCC: error: no matching function for call to 'cmp_greater\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'cmp_greater'
// [utility.intcmp]/6: cmp_greater is "Equivalent to: return cmp_less(u, t);" and /4 cmp_less
// "Mandates: Each of T and U is a signed or unsigned integer type"; Note 1: these templates
// "cannot be used to compare ... char16_t".
#include <utility>

bool b = std::cmp_greater(u'a', 1);
