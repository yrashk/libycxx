// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::format: invalid presentation type for the argument
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::format: invalid presentation type for the argument
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: invalid presentation type for the argument
// [format.string.std] Table 111: pointers accept only none, p and P.
#include <format>
#include <string>

int main() { (void)std::format("{:x}", nullptr); }
