// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::format: invalid presentation type for the argument
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::format: invalid presentation type for the argument
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: invalid presentation type for the argument
// [format.string.general]/5: "If format-spec does not conform to the format specifications
// for the argument type referred to by arg-id, the string is not a format string for args";
// [format.fmt.string]/3: the consteval constructor is then not a constant expression.
// Table 107 has no s type for integers.
#include <format>
#include <string>

int main() { (void)std::format("{:s}", 42); }
