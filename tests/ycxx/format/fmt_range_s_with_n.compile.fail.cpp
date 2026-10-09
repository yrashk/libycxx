// EXPECT-ERROR-GCC: error: uncaught exception of type 'std::format_error';[^\n]*std::range_formatter: n cannot be combined with s or \?s
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::range_formatter: n cannot be combined with s or \?s
// [format.range.formatter]/6: "If the range-type is s or ?s, then there shall be no n option
// and no range-underlying-spec."
#include <format>
#include <string>
#include <vector>

int main() { (void)std::format("{:ns}", std::vector<char>{'a'}); }
