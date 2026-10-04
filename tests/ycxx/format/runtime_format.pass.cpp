// [format.syn], [format.fmt.string]: runtime_format(s) yields a dynamic-format-string from
// which basic_format_string is constructible without compile-time checking; errors are then
// reported at run time by format_error ([format.err.report]).
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include "check.hpp"

static_assert(noexcept(std::runtime_format(std::string_view("x"))));
static_assert(noexcept(std::runtime_format(std::wstring_view(L"x"))));
static_assert(!std::is_copy_constructible_v<decltype(std::runtime_format(std::string_view("x")))>);

int main() {
  std::string fmt = "{}-{}";
  CHECK(std::format(std::runtime_format(fmt), 1, 2) == "1-2");
  CHECK(std::format(std::runtime_format(std::string_view("{:>3}")), 7) == "  7");
  CHECK(std::format(std::runtime_format(L"{}"), 5) == L"5");
  bool threw = false;
  try {
    (void)std::format(std::runtime_format("{:s}"), 1);
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(threw);
  threw = false;
  try {
    (void)std::format(std::runtime_format("{} {}"), 1);
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(threw);
  std::format_string<int> fs = std::runtime_format("{:x}");
  CHECK(fs.get() == "{:x}" && std::format(fs, 255) == "ff");
  char buf[8];
  auto r = std::format_to_n(buf, 8, std::runtime_format("{}"), 12);
  CHECK(r.size == 2);
}
