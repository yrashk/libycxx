// [utility.intcmp]/9: in_range "Mandates: Each of T and R is a signed or unsigned integer
// type"; Note 1: "These function templates cannot be used to compare ... char8_t".
#include <utility>

bool b = std::in_range<char8_t>(65);
