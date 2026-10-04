// [format.string.std] Table 106 and /15: strings (const char*, char*, char[N],
// basic_string, basic_string_view) are copied for type none or s; precision truncates; an
// embedded NUL in a string_view is copied; a char array formats its elements up to the
// terminating null ([format.formatter.spec] formatter<charT[N], charT>).
#include <format>
#include <string>
#include <string_view>
#include "check.hpp"

int main() {
  const char* cp = "hello";
  char buf[] = "mutable";
  char* mp = buf;
  CHECK(std::format("{}", cp) == "hello" && std::format("{:s}", cp) == "hello");
  CHECK(std::format("{}", mp) == "mutable");
  CHECK(std::format("{}", buf) == "mutable");
  CHECK(std::format("{}", std::string("str")) == "str");
  CHECK(std::format("{}", std::string_view("view")) == "view");
  CHECK(std::format("{:.3}", cp) == "hel");
  CHECK(std::format("{:.0}", cp) == "");
  CHECK(std::format("{:.10}", cp) == "hello");
  CHECK(std::format("{:>8.2}", std::string("abcd")) == "      ab");
  CHECK(std::format("{}", std::string_view("a\0b", 3)) == std::string("a\0b", 3));
  CHECK(std::format("{}|{}", "", std::string()) == "|");
  CHECK(std::format(L"{}", L"wide") == L"wide");
  CHECK(std::format(L"{:.2}", std::wstring(L"wide")) == L"wi");
  CHECK(std::format(L"{:>5}", std::wstring_view(L"ab")) == L"   ab");
  std::string s = std::format("{}{}{}", "a", std::string("b"), std::string_view("c"));
  CHECK(s == "abc");
}
