// [format.range.formatter]: range_formatter<T, charT> as a building block for program-defined
// formatters. /7-/8: set_separator / set_brackets; /5: n removes the brackets; Table 115: m
// sets "{", "}", ", " and formats the elements as if m were given for their tuple-type;
// /9: parse "Calls underlying_.parse(ctx) to parse format-spec in range-format-spec or, if the
// latter is not present, an empty format-spec. The values of opening-bracket_,
// closing-bracket_, and separator_ are modified if and only if required by the range-type or
// the n option", and set_debug_format() is called on the underlying formatter only when the
// range-type is neither s nor ?s and there is no range-underlying-spec; underlying() gives
// access to the element formatter (const and non-const). /11: the output, "adjusted
// according to the range-format-spec" (fill, align, width for the whole range; the default
// alignment is < for non-arithmetic types, [format.string.std] Table 104).
// [format.range.fmtdef]: nested ranges each parse their own range-format-spec, so "{:n:n}",
// "{:::#x}" and "{::*^8}" reach the inner levels. [format.tuple]/5-/7: a pair formatter used
// as the underlying formatter keeps brackets / separator set before parse.
// [format.formatter.spec]/2: set_debug_format() of the debug-enabled specializations makes
// the formatter act as if the parsed type were ?, keeping the other parsed options.
// fmt-maybe-const / input ranges: [format.range.fmtdef], views::istream is an input range.
#include <format>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

struct Bag {
  std::vector<int> v;
};
template <>
struct std::formatter<Bag, char> {
  std::range_formatter<int, char> rf;
  constexpr formatter() {
    rf.set_separator("|");
    rf.set_brackets("<", ">");
  }
  constexpr auto parse(std::format_parse_context& ctx) { return rf.parse(ctx); }
  auto format(const Bag& b, std::format_context& ctx) const { return rf.format(b.v, ctx); }
};

using P = std::pair<int, int>;
struct Pairs {
  std::vector<P> v;
};
template <>
struct std::formatter<Pairs, char> {
  std::range_formatter<P, char> rf;
  constexpr formatter() {
    rf.set_separator("; ");
    rf.underlying().set_brackets("<", ">");
    rf.underlying().set_separator("=");
  }
  constexpr auto parse(std::format_parse_context& ctx) { return rf.parse(ctx); }
  auto format(const Pairs& p, std::format_context& ctx) const { return rf.format(p.v, ctx); }
};

// A debug-enabled formatter used through set_debug_format() after parse.
struct Quoted {
  std::string_view s;
};
template <>
struct std::formatter<Quoted, char> : std::formatter<std::string_view, char> {
  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = std::formatter<std::string_view, char>::parse(ctx);
    this->set_debug_format();
    return it;
  }
  auto format(Quoted q, std::format_context& ctx) const { return std::formatter<std::string_view, char>::format(q.s, ctx); }
};
struct QChar {
  char c;
};
template <>
struct std::formatter<QChar, char> : std::formatter<char, char> {
  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = std::formatter<char, char>::parse(ctx);
    this->set_debug_format();
    return it;
  }
  auto format(QChar q, std::format_context& ctx) const { return std::formatter<char, char>::format(q.c, ctx); }
};

using RF = std::range_formatter<int, char>;
static_assert(std::is_same_v<decltype(std::declval<RF&>().underlying()), std::formatter<int, char>&>);
static_assert(std::is_same_v<decltype(std::declval<const RF&>().underlying()), const std::formatter<int, char>&>);
static_assert(noexcept(std::declval<RF&>().underlying()) && noexcept(std::declval<const RF&>().underlying()));

int main() {
  // Custom brackets and separator; the range-format-spec still applies.
  Bag b{{1, 2, 3}};
  CHECK(std::format("{}", b) == "<1|2|3>");
  CHECK(std::format("{:n}", b) == "1|2|3");             // n: brackets only
  CHECK(std::format("{::#x}", b) == "<0x1|0x2|0x3>");
  CHECK(std::format("{:*^11}", b) == "**<1|2|3>**");
  CHECK(std::format("{:9}", b) == "<1|2|3>  ");         // ranges are left-aligned by default
  CHECK(std::format("{}", Bag{}) == "<>");
  // The underlying pair formatter's own brackets and separator.
  Pairs ps{{{1, 2}, {3, 4}}};
  CHECK(std::format("{}", ps) == "[<1=2>; <3=4>]");    // the range's own brackets are the default
  CHECK(std::format("{:n}", ps) == "<1=2>; <3=4>");
  CHECK(std::format("{:m}", ps) == "{1: 2, 3: 4}");      // m: "{", "}", ", " and "k: v" elements
  CHECK(std::format("{:nm}", ps) == "1: 2, 3: 4");
  // set_debug_format keeps the other parsed options.
  CHECK(std::format("{}", Quoted{"a\tb"}) == "\"a\\tb\"");
  CHECK(std::format("{:>8}", Quoted{"abc"}) == "   \"abc\"");
  CHECK(std::format("{:.3}", Quoted{"abcdef"}) == "\"ab");
  CHECK(std::format("{:*<5}", QChar{'\n'}) == "'\\n'*");

  // Nested ranges: each level parses its own range-format-spec.
  std::vector<std::vector<int>> nv = {{1, 2}, {3}};
  CHECK(std::format("{}", nv) == "[[1, 2], [3]]");
  CHECK(std::format("{::n}", nv) == "[1, 2, 3]");
  CHECK(std::format("{:n:n}", nv) == "1, 2, 3");
  CHECK(std::format("{:n}", nv) == "[1, 2], [3]");
  CHECK(std::format("{:::#x}", nv) == "[[0x1, 0x2], [0x3]]");
  CHECK(std::format("{::*^8}", nv) == "[*[1, 2]*, **[3]***]");
  CHECK(std::format("{:>16::02}", nv) == "[[01, 02], [03]]");
  CHECK(std::format("{:>18::02}", nv) == "  [[01, 02], [03]]");
  std::vector<std::vector<std::string>> ns = {{"a"}, {"b\n"}};
  CHECK(std::format("{}", ns) == "[[\"a\"], [\"b\\n\"]]");
  CHECK(std::format("{::}", ns) == "[[\"a\"], [\"b\\n\"]]");  // the middle level has no underlying spec
  CHECK(std::format("{:::}", ns) == "[[a], [b\n]]");         // the innermost has an (empty) one
  std::vector<std::vector<char>> nc = {{'a', 'b'}, {'c'}};
  CHECK(std::format("{::s}", nc) == "[ab, c]");
  CHECK(std::format("{::?s}", nc) == "[\"ab\", \"c\"]");
  CHECK(std::format("{}", nc) == "[['a', 'b'], ['c']]");
  CHECK(std::format("{:::d}", nc) == "[[97, 98], [99]]");
  // s / ?s with fill, alignment and width (formatted as a string: left-aligned by default).
  std::vector<char> ab = {'a', '"'};
  CHECK(std::format("{:5s}|", ab) == "a\"   |");
  CHECK(std::format("{:*^8?s}", ab) == "*\"a\\\"\"**");
  // Maps: n removes the braces; elements are still "k: v".
  std::vector<std::pair<std::string, int>> kv = {{"x", 1}};
  CHECK(std::format("{:m}", kv) == "{\"x\": 1}");
  CHECK(std::format("{::}", kv) == "[(\"x\", 1)]");          // pair elements are always debug ([format.tuple]/7)
  // An input-only range.
  std::istringstream in("4 5 6");
  auto iv = std::views::istream<int>(in);
  CHECK(std::format("{}", iv) == "[4, 5, 6]");
  // Wide.
  CHECK(std::format(L"{:n:n}", nv) == L"1, 2, 3");
  CHECK(std::format(L"{}", std::vector<std::wstring>{L"a\t"}) == L"[\"a\\t\"]");
  CHECK(std::format(L"{::?s}", std::vector<std::vector<wchar_t>>{{L'x'}}) == L"[\"x\"]");
  return 0;
}
