// [utility.intcmp]/1: cmp_equal "Mandates: Each of T and U is a signed or unsigned integer
// type"; Note 1: "These function templates cannot be used to compare ... bool."
#include <utility>

bool b = std::cmp_equal(true, 1);
