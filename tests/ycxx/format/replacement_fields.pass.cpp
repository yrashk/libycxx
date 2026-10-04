// [format.string.general]: escape sequences {{ and }}, automatic indexing (0, 1, 2, ...),
// manual indexing (arguments may be reused or skipped), and characters copied unchanged.
#include <format>
#include <string>
#include "check.hpp"

int main() {
  CHECK(std::format("{0}-{{", 8) == "8-{");
  CHECK(std::format("{{}}") == "{}");
  CHECK(std::format("}}{{x") == "}{x");
  CHECK(std::format("plain text") == "plain text");
  CHECK(std::format("") == "");
  CHECK(std::format("{} to {}", "a", "b") == "a to b");
  CHECK(std::format("{1} to {0}", "a", "b") == "b to a");
  CHECK(std::format("{0}{0}{1}{0}", 'x', 'y') == "xxyx");
  CHECK(std::format("{1}", 1, 2, 3) == "2"); // unused arguments are fine
  CHECK(std::format("{}", 1, 2) == "1");
  CHECK(std::format("{{{}}}", 5) == "{5}");
  CHECK(std::format("{:}", 7) == "7"); // empty format-spec
  CHECK(std::format("{0:}|{0}", 7) == "7|7");
  CHECK(std::format("{10}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == "10");
}
