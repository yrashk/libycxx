// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::format: the sign and \# options need an arithmetic presentation
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::format: the sign and \# options need an arithmetic presentation
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: the sign and \# options need an arithmetic presentation
// [format.string.std]/5: "The sign option is only valid for arithmetic types other than
// charT and bool or when an integer presentation type is specified."
#include <format>
#include <string>

int main() { (void)std::format("{:+}", "text"); }
