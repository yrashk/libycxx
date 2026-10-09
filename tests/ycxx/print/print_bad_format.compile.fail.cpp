// EXPECT-ERROR-GCC: error: uncaught exception of type 'std::format_error';[^\n]*std::format: invalid format specification
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::format: invalid format specification
// [print.syn]: print takes a format_string<Args...>; [format.fmt.string]/2: its consteval
// constructor requires the string to be a format string for Args ("Remarks: A call to this
// function is not a core constant expression ([expr.const]) unless there exist args of types
// Args such that str is a format string for args"). An invalid specification for an int is
// ill-formed.
#include <print>
#include <sstream>

void f(std::ostringstream& os) {
  std::print(os, "{:q}", 1);
}
