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
