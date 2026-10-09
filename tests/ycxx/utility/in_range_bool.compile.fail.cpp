// EXPECT-ERROR-GCC: error: no matching function for call to 'in_range<bool>\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'in_range'
// [utility.intcmp]/9: in_range<R>(t): "Mandates: Each of T and R is a signed or unsigned
// integer type." Note 1: these templates "cannot be used to compare ... bool". (R = bool)
#include <utility>

bool b = std::in_range<bool>(1);
