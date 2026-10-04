// Ranges produced by range adaptors, and nested ranges with a format-spec at each level.
// [format.range.fmtkind]/1-2: format_kind is sequence for these views (their reference type
// is not the view itself and they have no key_type), map / set for map and set.
// [format.syn]: fmt-maybe-const<R, charT> is const R only when const R is a
// const-formattable-range; [format.range.fmtdef]/4: format(maybe-const-r& elems, ...), so a
// view that is not const-iterable (filter_view, basic_istream_view) is formattable as a
// non-const lvalue only. [format.range.formatter]/2-3: the range-underlying-spec is parsed by
// the element formatter, which for a range element is again a range formatter, so each ':'
// descends one level; /5: n removes the brackets of its own level; /9: debug format for the
// elements unless a range-underlying-spec is present; /11.1-11.2: s / ?s format a char range
// as a string. Table 115: m needs a pair or 2-tuple element type (zip and enumerate yield
// tuple<A&, B&> / tuple<D, T&>) and writes "{k: v, ...}". [format.tuple]/2-7: tuple
// elements are always debug-formatted. [format.range.fmtmap]/2: "{k: v}" for maps.
#include <array>
#include <format>
#include <map>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "check.hpp"

namespace views = std::views;

int main() {
  // Nested ranges and a spec per level.
  const std::vector<std::vector<int>> vv{{1, 2}, {3}};
  CHECK(std::format("{}", vv) == "[[1, 2], [3]]");
  CHECK(std::format("{}", std::vector<std::vector<int>>{{}}) == "[[]]");
  CHECK(std::format("{::n}", vv) == "[1, 2, 3]");             // the inner level loses its brackets
  CHECK(std::format("{:n}", vv) == "[1, 2], [3]");             // the outer level does
  CHECK(std::format("{:n:n:#x}", vv) == "0x1, 0x2, 0x3");      // three levels
  CHECK(std::format("{:::#x}", vv) == "[[0x1, 0x2], [0x3]]");
  CHECK(std::format("{:*>18:->6}", std::vector<std::vector<int>>{{1}, {2}}) == "**[---[1], ---[2]]");
  CHECK(std::format("{:::*^3}", vv) == "[[*1*, *2*], [*3*]]");
  const std::vector<std::vector<std::vector<char>>> vvv{{{'a', 'b'}}, {}};
  CHECK(std::format("{}", vvv) == "[[['a', 'b']], []]");
  CHECK(std::format("{:::s}", vvv) == "[[ab], []]");
  CHECK(std::format("{:::?s}", vvv) == "[[\"ab\"], []]");
  const std::vector<std::array<char, 2>> va{{'x', '\n'}};
  CHECK(std::format("{}", va) == "[['x', '\\n']]");
  CHECK(std::format("{::?s}", va) == "[\"x\\n\"]");
  CHECK(std::format("{::s}", va) == "[x\n]");

  // join, chunk, split, zip, enumerate, transform.
  CHECK(std::format("{}", vv | views::join) == "[1, 2, 3]");
  const std::vector<int> v{1, 2, 3, 4, 5};
  CHECK(std::format("{}", v | views::chunk(2)) == "[[1, 2], [3, 4], [5]]");
  CHECK(std::format("{:n:n}", v | views::chunk(2)) == "1, 2, 3, 4, 5");
  constexpr std::string_view csv = "ab,c\"d";
  CHECK(std::format("{}", csv | views::split(',')) == "[['a', 'b'], ['c', '\"', 'd']]");
  CHECK(std::format("{::s}", csv | views::split(',')) == "[ab, c\"d]");
  CHECK(std::format("{::?s}", csv | views::split(',')) == "[\"ab\", \"c\\\"d\"]");
  const std::vector<char> cs{'x', 'y'};
  CHECK(std::format("{}", views::zip(v, cs)) == "[(1, 'x'), (2, 'y')]");
  CHECK(std::format("{:m}", views::zip(v, cs)) == "{1: 'x', 2: 'y'}");
  CHECK(std::format("{:n}", views::zip(cs, v)) == "('x', 1), ('y', 2)");
  const std::vector<std::string> words{"a", "b\t"};
  CHECK(std::format("{:m}", views::enumerate(words)) == "{0: \"a\", 1: \"b\\t\"}");
  CHECK(std::format("{}", v | views::transform([](int i) { return i * 10; }) | views::take(2)) == "[10, 20]");
  CHECK(std::format("{}", views::iota(1, 4) | views::reverse) == "[3, 2, 1]");
  CHECK(std::format("{}", views::repeat('z', 2)) == "['z', 'z']");
  CHECK(std::format("{::d}", views::repeat('z', 2)) == "[122, 122]");

  // Not const-iterable: formattable as a non-const lvalue only.
  auto odd = v | views::filter([](int i) { return i % 2 != 0; });
  using Odd = decltype(odd);
  static_assert(std::formattable<Odd, char> && !std::formattable<const Odd, char>);
  CHECK(std::format("{}", odd) == "[1, 3, 5]");
  CHECK(std::format("{:n:02}", odd) == "01, 03, 05");
  std::istringstream in("4 5 6");
  auto iv = views::istream<int>(in);
  static_assert(!std::formattable<const decltype(iv), char>);
  CHECK(std::format("{:n}", iv) == "4, 5, 6");

  // Maps and sets of ranges and pairs.
  const std::map<std::string, std::vector<int>> m{{"a", {1, 2}}, {"b", {}}};
  CHECK(std::format("{}", m) == "{\"a\": [1, 2], \"b\": []}");
  CHECK(std::format("{:n}", m) == "\"a\": [1, 2], \"b\": []");
  const std::set<std::pair<int, char>> sp{{2, 'b'}, {1, 'a'}};
  CHECK(std::format("{}", sp) == "{(1, 'a'), (2, 'b')}");
  CHECK(std::format("{:m}", sp) == "{1: 'a', 2: 'b'}");
  const std::vector<std::map<int, int>> vm{{{1, 2}}, {}};
  CHECK(std::format("{}", vm) == "[{1: 2}, {}]");

  // Wide.
  CHECK(std::format(L"{:n:n}", vv) == L"1, 2, 3");
  CHECK(std::format(L"{:m}", views::zip(v, cs)) == L"{1: 'x', 2: 'y'}");
  CHECK(std::format(L"{::s}", std::wstring_view(L"ab,c") | views::split(L',')) == L"[ab, c]");
  return 0;
}
