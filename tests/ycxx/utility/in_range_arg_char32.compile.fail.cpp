// EXPECT-ERROR-GCC: error: no matching function for call to 'in_range<int>\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'in_range'
// [utility.intcmp]/9: in_range "Mandates: Each of T and R is a signed or unsigned integer type";
// here the argument type T is char32_t (R = int is valid). Control: intcmp_matrix.pass.cpp.
#include <utility>

bool b = std::in_range<int>(U'a');
