// [format.range]: ranges format as [e1, e2, ...] with elements in debug format unless a
// range-underlying-spec is given ([format.range.formatter]/9); n removes the brackets (/5);
// m formats pair elements map-style (Table 115); s and ?s format char ranges as (escaped)
// strings (/11); width, fill and alignment apply to the whole range; format_kind gives map
// and set kinds for associative containers ([format.range.fmtkind]/2), formatted with {}
// and k: v; views are formattable; set_separator/set_brackets of range_formatter.
#include <array>
#include <format>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "check.hpp"

static_assert(std::format_kind<std::vector<int>> == std::range_format::sequence);
static_assert(std::format_kind<std::map<int, int>> == std::range_format::map);
static_assert(std::format_kind<std::set<int>> == std::range_format::set);
static_assert(std::formattable<std::vector<std::string>, char>);
static_assert(std::formattable<std::array<int, 2>, char>);

int main() {
  std::vector<int> v = {1, 2, 3};
  CHECK(std::format("{}", v) == "[1, 2, 3]");
  CHECK(std::format("{:n}", v) == "1, 2, 3");
  CHECK(std::format("{::02}", v) == "[01, 02, 03]");
  CHECK(std::format("{:n:#x}", v) == "0x1, 0x2, 0x3");
  CHECK(std::format("{:*^13}", v) == "**[1, 2, 3]**");
  CHECK(std::format("{:>12}", v) == "   [1, 2, 3]");
  CHECK(std::format("{}", std::vector<int>{}) == "[]");
  CHECK(std::format("{}", std::vector<std::string>{"a", "b\n"}) == "[\"a\", \"b\\n\"]");
  CHECK(std::format("{::}", std::vector<std::string>{"a", "b"}) == "[a, b]"); // underlying spec: no debug
  std::vector<char> cs = {'h', 'i', '\t'};
  CHECK(std::format("{}", cs) == "['h', 'i', '\\t']");
  CHECK(std::format("{:s}", cs) == "hi\t");
  CHECK(std::format("{:?s}", cs) == "\"hi\\t\"");
  CHECK(std::format("{:>6s}", std::vector<char>{'a', 'b'}) == "    ab");
  std::vector<std::vector<int>> nested = {{1}, {}, {2, 3}};
  CHECK(std::format("{}", nested) == "[[1], [], [2, 3]]");
  std::vector<std::pair<int, std::string>> ps = {{1, "one"}, {2, "two"}};
  CHECK(std::format("{}", ps) == "[(1, \"one\"), (2, \"two\")]");
  CHECK(std::format("{:m}", ps) == "{1: \"one\", 2: \"two\"}");
  CHECK(std::format("{:nm}", ps) == "1: \"one\", 2: \"two\"");
  std::map<std::string, int> m = {{"a", 1}, {"b", 2}};
  CHECK(std::format("{}", m) == "{\"a\": 1, \"b\": 2}");
  CHECK(std::format("{}", std::map<int, int>{}) == "{}");
  std::set<int> s = {3, 1, 2};
  CHECK(std::format("{}", s) == "{1, 2, 3}");
  CHECK(std::format("{:n}", s) == "1, 2, 3");
  CHECK(std::format("{}", std::views::iota(1, 4)) == "[1, 2, 3]");
  CHECK(std::format("{}", std::views::iota(1, 6) | std::views::filter([](int x) { return x % 2; })) == "[1, 3, 5]");
  std::array<double, 2> a = {0.5, 1.0};
  CHECK(std::format("{::.2f}", a) == "[0.50, 1.00]");
  CHECK(std::format(L"{}", std::vector<int>{4, 5}) == L"[4, 5]");
  std::range_formatter<int> rf;
  rf.set_separator("; ");
  rf.set_brackets("<", ">");
  static_assert(noexcept(rf.set_separator("")) && noexcept(rf.set_brackets("", "")));
  static_assert(std::is_same_v<decltype(rf.underlying()), std::formatter<int, char>&>);
}
