// EXPECT-ERROR-GCC: error: no matching function for call to 'cmp_not_equal\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'cmp_not_equal'
// [utility.intcmp]/3: cmp_not_equal is "Equivalent to: return !cmp_equal(t, u);" and /1
// cmp_equal "Mandates: Each of T and U is a signed or unsigned integer type"; Note 1: these
// templates "cannot be used to compare ... wchar_t".
#include <utility>

bool b = std::cmp_not_equal(1, L'a');
