// EXPECT-ERROR-GCC: error: uncaught exception of type 'std::format_error';[^\n]*std::format: precision is valid only for floating\-point and string types
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: precision is valid only for floating\-point and string types
// [format.string.std]/15: "The precision option is valid for floating-point and string
// types." An integer argument with a precision is not a format string.
#include <format>
#include <string>

int main() { (void)std::format("{:.2}", 1); }
