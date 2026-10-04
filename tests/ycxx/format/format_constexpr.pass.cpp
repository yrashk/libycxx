// [format.functions]/1: "A call to any of the functions defined in this subclause is a
// constant subexpression only if each of the used formatter specializations is a
// constexpr-enabled specialization"; format, vformat, format_to, format_to_n and
// formatted_size are constexpr, and the char, string, integer, bool and nullptr_t
// formatters are constexpr-enabled ([format.formatter.spec]/2).
#include <format>
#include <string>
#include <string_view>
#include "check.hpp"

constexpr bool test() {
  CHECK(std::format("{}-{:>4}-{:#x}", 12, 'c', 255) == "12-   c-0xff");
  CHECK(std::format("{} {:?}", std::string_view("sv"), "q\n") == "sv \"q\\n\"");
  CHECK(std::format("{:+d} {}", true, false) == "+1 false");
  CHECK(std::format("{}", nullptr) == "0x0");
  CHECK(std::formatted_size("{:08b}", 5u) == 8);
  char buf[8] = {};
  auto r = std::format_to_n(buf, 3, "{}", 123456);
  CHECK(r.size == 6 && buf[2] == '3');
  char* e = std::format_to(buf, "{}", -7);
  CHECK(e == buf + 2 && buf[0] == '-');
  CHECK(std::format(L"{}", 42) == L"42");
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
