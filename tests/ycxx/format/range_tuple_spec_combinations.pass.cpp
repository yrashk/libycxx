// [format.range.formatter]/5-11: range-format-spec is range-fill-and-align, width, n, range-type
// and then ':' format-spec for the underlying formatter; width may be a nested replacement field
// (dynamic), and so may the underlying formatter's; "n" drops the opening and closing brackets;
// "s" / "?s" format a range of charT as a string (escaped with ?); "m" is valid for pairs and
// 2-tuples. The underlying formatter's parse sees the text after the second ':' so nested ranges
// take their own n / range-type / underlying spec. [format.range.fmtmap]/[format.range.fmtset]:
// map-like ranges use "{", "}" and ": ", set-like ones "{", "}". [format.tuple]/4-9: the tuple
// formatter has fill-and-align, width and n / m; its elements are formatted with set_debug_format
// when available. [format.string.escaped]: in a char, ' is escaped and " is not; in a string, "
// is escaped and ' is not; a backslash is always escaped.
// [format.functions]: formatted_size is the length of format's result; format_to_n writes at most
// n characters and returns the full size.
#include <format>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include "check.hpp"

int main() {
  const std::vector<int> v{1, 2, 3};
  // Dynamic widths, for the range and for its elements.
  CHECK(std::format("{:*^{}}", v, 15) == "***[1, 2, 3]***");
  CHECK(std::format("{::{}}", v, 3) == "[  1,   2,   3]");
  CHECK(std::format("{0::{1}}|{0:>{1}}", std::vector<int>{7}, 4) == "[   7]| [7]");
  CHECK(std::format("{:-<12n:+}", v) == "+1, +2, +3--");
  // Nested ranges with their own options.
  const std::vector<std::vector<int>> vv{{1, 2}, {3}};
  CHECK(std::format("{:n:n:#x}", vv) == "0x1, 0x2, 0x3");
  CHECK(std::format("{::n}", vv) == "[1, 2, 3]");
  CHECK(std::format("{:n:}", vv) == "[1, 2], [3]");
  CHECK(std::format("{:::>2}", vv) == "[[ 1,  2], [ 3]]");
  const std::vector<std::vector<char>> vc{{'a', 'b'}, {'"'}};
  CHECK(std::format("{::s}", vc) == "[ab, \"]");
  CHECK(std::format("{::?s}", vc) == "[\"ab\", \"\\\"\"]");
  CHECK(std::format("{}", vc) == "[['a', 'b'], ['\"']]");
  // Escaping: ' in a char, " in a string, backslash in both.
  CHECK(std::format("{}", std::vector<char>{'\'', '"', '\\'}) == "['\\'', '\"', '\\\\']");
  CHECK(std::format("{}", std::vector<std::string>{"'\"\\"}) == "[\"'\\\"\\\\\"]");
  CHECK(std::format("{}", std::vector<const char*>{"x\ty"}) == "[\"x\\ty\"]");
  CHECK(std::format("{}", std::vector<std::string_view>{"", "\r"}) == "[\"\", \"\\r\"]");
  // Pairs inside ranges with the m option on the element.
  const std::vector<std::pair<int, char>> pc{{1, 'a'}, {2, 'b'}};
  CHECK(std::format("{::m}", pc) == "[1: 'a', 2: 'b']");
  CHECK(std::format("{::n}", pc) == "[1, 'a', 2, 'b']");
  CHECK(std::format("{:n:>10}", pc) == "  (1, 'a'),   (2, 'b')");
  // Maps and sets.
  const std::map<int, std::string> m{{1, "x"}, {2, "y"}};
  CHECK(std::format("{:n}", m) == "1: \"x\", 2: \"y\"");
  CHECK(std::format("{:*>18}", m) == "**{1: \"x\", 2: \"y\"}");
  CHECK(std::format("{::}", std::set<char>{'b', 'a'}) == "{a, b}");
  CHECK(std::format("{:s}", std::set<char>{'b', 'a'}) == "ab");
  CHECK(std::format("{::d}", std::vector<bool>{true, false}) == "[1, 0]");
  CHECK(std::format("{::d}", std::set<bool>{true, false}) == "{0, 1}");
  // Tuples.
  CHECK(std::format("{:>12}", std::pair(std::string("a"), 'b')) == "  (\"a\", 'b')");
  CHECK(std::format("{:n}", std::tuple<>()) == "");
  CHECK(std::format("{:^{}n}", std::tuple(1, 2, 3), 11) == "  1, 2, 3  ");
  CHECK(std::format("{}", std::tuple(std::vector<int>{1}, std::pair('\n', "\n"))) ==
        "([1], ('\\n', \"\\n\"))");
  // formatted_size and format_to_n agree with format.
  CHECK(std::formatted_size("{}", v) == 9);
  CHECK(std::formatted_size("{:n:#x}", v) == 13);
  char buf[16] = {};
  auto r = std::format_to_n(buf, 4, "{}", v);
  CHECK(r.size == 9 && r.out == buf + 4 && std::string_view(buf, 4) == "[1, ");
  r = std::format_to_n(buf, 0, "{}", pc);
  CHECK(r.size == 20 && r.out == buf);
  // Wide.
  CHECK(std::format(L"{}", std::vector<std::wstring>{L"a\"", L""}) == L"[\"a\\\"\", \"\"]");
  CHECK(std::format(L"{::m}", std::vector<std::pair<int, wchar_t>>{{1, L'w'}}) == L"[1: 'w']");
  return 0;
}
