// EXPECT-ERROR-GCC: error: uncaught exception of type 'std::format_error';[^\n]*std::range_formatter: s and \?s need a range of the character type
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::range_formatter: s and \?s need a range of the character type
// [format.range.formatter] Table 115: the s range-type requires "T shall be charT".
#include <format>
#include <string>
#include <vector>

int main() { (void)std::format("{:s}", std::vector<int>{1, 2}); }
