// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::format: invalid presentation type for the argument
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::format: invalid presentation type for the argument
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: invalid presentation type for the argument
// [format.range.formatter]/3, /9: the format-spec of a range-underlying-spec is parsed by
// formatter<T, charT>; for T = int, "s" is not a valid type (Table 107), so "{::s}" with a
// vector<int> is not a format string ([format.string.general]/5, [format.fmt.string]/3).
// The control uses the x type.
#include <format>
#include <vector>
int main() {
  (void)std::format("{::x}", std::vector<int>{1});  // control
#ifndef YCXX_CONTROL
  (void)std::format("{::s}", std::vector<int>{1});
#endif
}
