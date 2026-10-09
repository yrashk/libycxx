// EXPECT-ERROR-GCC: error: uncaught exception of type 'std::format_error';[^\n]*std::formatter: the m tuple\-type needs exactly two elements
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::formatter: the m tuple\-type needs exactly two elements
// [format.tuple] Table 116: the m tuple-type requires sizeof...(Ts) == 2.
#include <format>
#include <string>
#include <tuple>

int main() { (void)std::format("{:m}", std::tuple<int, int, int>(1, 2, 3)); }
