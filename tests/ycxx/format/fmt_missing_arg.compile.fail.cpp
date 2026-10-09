// EXPECT-ERROR-GCC: error: call to non-'constexpr' function [^\n]*__format_string_argument_index_out_of_range\(\)
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: non-constexpr function '__format_string_argument_index_out_of_range' cannot be used in a constant expression
// [format.string.general]/2: "If there is no argument with the index arg-id in args, the
// string is not a format string for args." ([format.fmt.string]/3: ill-formed.)
#include <format>
#include <string>

int main() { (void)std::format("{} {}", 1); }
