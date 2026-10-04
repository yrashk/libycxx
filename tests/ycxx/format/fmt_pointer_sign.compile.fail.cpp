// [format.string.std]/5: "The sign option is only valid for arithmetic types other than charT
// and bool or when an integer presentation type is specified"; pointer types (Table 111:
// none, p, P) are neither, so "{:+}" with nullptr is not a format string
// ([format.string.general]/5, [format.fmt.string]/3). The control uses 0, which /8 allows for
// pointer types.
#include <format>

int main() {
  (void)std::format("{:08}", nullptr);  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:+}", nullptr);
#endif
}
