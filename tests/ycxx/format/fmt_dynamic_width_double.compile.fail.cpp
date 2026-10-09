// EXPECT-ERROR-GCC: error: call to non-'constexpr' function [^\n]*__format_string_dynamic_argument_has_wrong_type\(\)
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: non-constexpr function '__format_string_dynamic_argument_has_wrong_type' cannot be used in a constant expression
// [format.string.std]/10: a dynamic width "is valid only if the corresponding formatting
// argument is of standard signed or unsigned integer type."
#include <format>
#include <string>

int main() { (void)std::format("{:{}}", 1, 2.0); }
