// EXPECT-ERROR-GCC: error: no matching function for call to 'cmp_equal\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'cmp_equal'
// [utility.intcmp]/1: cmp_equal "Mandates: Each of T and U is a signed or unsigned integer type";
// Note 1: "These function templates cannot be used to compare byte, char, char8_t, char16_t,
// char32_t, wchar_t, and bool." char8_t is unsigned-like but is not an unsigned integer type
// ([basic.fundamental]/2). Control: intcmp_matrix.pass.cpp (unsigned char, unsigned).
#include <utility>

bool b = std::cmp_equal(u8'a', 97u);
