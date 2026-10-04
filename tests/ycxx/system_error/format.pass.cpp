// [syserr.fmt]: formatter<error_code, charT>. error-code-format-spec is
// fill-and-align_opt width_opt ?_opt s_opt. /5: with s, msg is ec.message() (for char and a
// UTF-8 literal encoding, transcoded to UTF-8); otherwise msg is
// format("{}:{}", ec.category().name(), ec.value()). With ?, msg is formatted as an escaped
// string ([format.string.escaped]). msg is written adjusted per the spec.
#include <system_error>
#include <format>
#include <string>
#include "check.hpp"

struct Cat : std::error_category {
  const char* name() const noexcept override { return "cat"; }
  std::string message(int ev) const override { return ev == 1 ? "tab\there" : "plain msg"; }
};

int main() {
  Cat c;
  std::error_code e1(1, c), e2(-25, c);
  CHECK(std::format("{}", e2) == "cat:-25");
  CHECK(std::format("{}", std::error_code()) == "system:0");
  CHECK(std::format("{:s}", e2) == "plain msg");
  CHECK(std::format("{:s}", e1) == "tab\there");
  CHECK(std::format("{:?s}", e1) == "\"tab\\there\"");
  CHECK(std::format("{:?}", e2) == "\"cat:-25\"");
  CHECK(std::format("{:>10}", e2) == "   cat:-25");
  CHECK(std::format("{:*<9}", e2) == "cat:-25**");
  CHECK(std::format("{:^11s}", e2) == " plain msg ");
  // msg is a string, written like one: without an alignment it is left-aligned
  // ([format.string.std] Table 104: "the default for ... strings" is left)
  CHECK(std::format("{:{}}", e2, 8) == "cat:-25 ");
  return 0;
}
