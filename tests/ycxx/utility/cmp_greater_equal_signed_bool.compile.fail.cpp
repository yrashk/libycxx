// [utility.intcmp]/7 (cmp_greater_equal is defined via cmp_less, whose Mandates apply):
// "Each of T and U is a signed or unsigned integer type"; bool is neither ([basic.fundamental]/2).
// Control: intcmp_matrix.pass.cpp.
#include <utility>

bool b = std::cmp_greater_equal(1L, true);
