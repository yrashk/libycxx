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
