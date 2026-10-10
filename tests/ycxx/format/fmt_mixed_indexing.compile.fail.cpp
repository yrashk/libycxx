// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::format: automatic and manual argument indexing are mixed
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::format: automatic and manual argument indexing are mixed
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: automatic and manual argument indexing are mixed
// [format.string.general]/4, Example 2: format("{0} to {}", "a", "b") is "not a format
// string (mixing automatic and manual indexing), ill-formed".
#include <format>
#include <string>

int main() { (void)std::format("{0} to {}", "a", "b"); }
