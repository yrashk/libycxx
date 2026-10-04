// [format.string.std] Table 109: the bool presentation types are none, s, b, B, d, o, x and
// X; c is not among them, so "{:c}" with a bool is not a format string
// ([format.string.general]/5, [format.fmt.string]/3). The control uses d.
#include <format>

int main() {
  (void)std::format("{:d}", true);  // control
#ifndef YCXX_CONTROL
  (void)std::format("{:c}", true);
#endif
}
