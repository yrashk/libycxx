// EXPECT-ERROR-GCC: error: uncaught exception of type 'std::format_error';[^\n]*std::format: the L option is valid only for arithmetic types
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: the L option is valid only for arithmetic types
// [format.string.std]/17: "The L option is only valid for arithmetic types"; a std-format-spec
// that is not valid for the argument makes the format string invalid, and format_string's
// consteval constructor makes the call ill-formed ([format.fmt.string]/3). The control (L with
// an int) is well-formed.
#include <format>
#include <string>

int main() {
  (void)std::format("{:L}", 12);  // control
  std::string s = "x";
#ifndef YCXX_CONTROL
  (void)std::format("{:L}", s);
#endif
}
