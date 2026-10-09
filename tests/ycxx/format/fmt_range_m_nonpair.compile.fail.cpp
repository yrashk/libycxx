// EXPECT-ERROR-GCC[exceptions]: error: uncaught exception of type 'std::format_error';[^\n]*std::range_formatter: m needs elements that are pairs or 2\-tuples
// EXPECT-ERROR-GCC[!exceptions]: error: call to non-'constexpr' function 'void ycxx_error_handler\(ycxx_error_kind, const char\*\)'
// EXPECT-ERROR-GCC[!exceptions]: in 'constexpr' expansion of [^\n]*std::range_formatter: m needs elements that are pairs or 2\-tuples
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::range_formatter: m needs elements that are pairs or 2\-tuples
// [format.range.formatter] Table 115: the m range-type requires T to be "either a
// specialization of pair or a specialization of tuple such that tuple_size_v<T> is 2".
#include <format>
#include <string>
#include <vector>

int main() { (void)std::format("{:m}", std::vector<int>{1, 2}); }
