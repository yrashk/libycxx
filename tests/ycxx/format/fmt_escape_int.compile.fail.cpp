// [format.string.std] Table 107 (integral presentation types: b, B, c, d, o, x, X, none) has no
// '?'; '?' is only a string / character presentation type (Tables 106, 108). An invalid type
// for the argument makes the format string invalid and the call ill-formed
// ([format.fmt.string]/3). The control ('?' with a char) is well-formed.
#include <format>

int main() {
  (void)std::format("{:?}", 'c');  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:?}", 42);
#endif
}
