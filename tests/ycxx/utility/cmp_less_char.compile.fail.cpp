// [utility.intcmp]/4: cmp_less "Mandates: Each of T and U is a signed or unsigned integer
// type"; Note 1: "These function templates cannot be used to compare ... char".
#include <utility>

bool b = std::cmp_less(1, 'a');
