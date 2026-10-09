// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::format: unmatched '\}' in the format string
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::format: unmatched \\'\}\\' in the format string
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: unmatched '\}' in the format string
// [format.string.general]/1: a lone } is neither an escape sequence nor part of a
// replacement field, so the string is not a format string.
#include <format>
#include <string>

int main() { (void)std::format("a } b", 1); }
