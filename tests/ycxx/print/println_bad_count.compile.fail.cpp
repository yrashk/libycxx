// EXPECT-ERROR-GCC: error: call to non-'constexpr' function [^\n]*__format_string_argument_index_out_of_range\(\)
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: non-constexpr function '__format_string_argument_index_out_of_range' cannot be used in a constant expression
// [format.fmt.string]/2: the format string is checked at compile time: a replacement field
// referring to a missing argument makes the call ill-formed.
#include <print>
#include <cstdio>

void f() {
  std::println(stdout, "{} {}", 1);
}
