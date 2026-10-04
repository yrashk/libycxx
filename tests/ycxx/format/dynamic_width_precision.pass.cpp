// [format.string.std]/10: "If { arg-id(opt) } is used in a width or precision option, the
// value of the corresponding formatting argument is used as the value of the option. The
// option is valid only if the corresponding formatting argument is of standard signed or
// unsigned integer type." Automatic and manual arg-ids inside the format-spec.
#include <format>
#include <string>
#include "check.hpp"

int main() {
  CHECK(std::format("{:{}}", 5, 4) == "   5");
  CHECK(std::format("{:*^{}}", "x", 5) == "**x**");
  CHECK(std::format("{0:{1}}|{0:{2}}", 1, 3, 2) == "  1| 1");
  CHECK(std::format("{:.{}f}", 3.14159, 2) == "3.14");
  CHECK(std::format("{:{}.{}}", "abcdef", 5, 3) == "abc  ");
  CHECK(std::format("{0:{2}.{1}}", "abcdef", 2, 4) == "ab  ");
  CHECK(std::format("{:{}}", 1, 0U) == "1");
  CHECK(std::format("{:{}}", 1, 3LL) == "  1");
  CHECK(std::format("{:{}}", 'c', static_cast<unsigned char>(2)) == "c ");
}
