// [format.string.std]/1: fill-and-align is "fill_opt align", fill is "any character other than
// { or }"; /3: "For a format specification in UTF-8, UTF-16, or UTF-32, the fill character
// corresponds to a single Unicode scalar value" (so a fill can take 2, 3 or 4 UTF-8 code
// units); Note 4: "fill characters are assumed to have a field width of 1"; Table 104: the
// default alignment per type and the floor(n/2) / ceil(n/2) split of ^; /8: the 0 option
// inserts no zeros when an align option is present. The same fill rules apply to the
// range-fill ([format.range.formatter]/2, /4: "any character other than { or } or :"), the
// tuple-fill ([format.tuple]/2-3) and the fill of a range-underlying-spec.
// (UTF-8 literal encoding; wchar_t is UTF-32 on this platform.)
#include <format>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "check.hpp"

int main() {
  // 2-, 3- and 4-code-unit fills, for every kind of argument and every alignment.
  CHECK(std::format("{:é^7}", "ab") == "ééabééé");           // n = 5: 2 before, 3 after
  CHECK(std::format("{:é<4}", "ab") == "abéé");
  CHECK(std::format("{:é>4}", "ab") == "ééab");
  CHECK(std::format("{:中>4}", 7) == "中中中7");             // the fill counts 1, not 2
  CHECK(std::format("{:中<4}", -7) == "-7中中");
  CHECK(std::format("{:中^5}", 7) == "中中7中中");
  CHECK(std::format("{:🤡<6}", true) == "true🤡🤡");
  CHECK(std::format("{:🤡>6}", false) == "🤡false");
  CHECK(std::format("{:ñ>3}", 'x') == "ññx");
  CHECK(std::format("{:ñ^4}", 'x') == "ñxññ");
  CHECK(std::format("{:é<5}", nullptr) == "0x0éé");
  CHECK(std::format("{:€^9.2f}", 3.14159) == "€€3.14€€€");
  CHECK(std::format("{:€>8}", 1.5) == "€€€€€1.5");
  CHECK(std::format("{:€<7e}", 0.0) == "0.000000e+00");  // wider than 7: no fill
  // The defaults (Table 104) are unchanged by a fill-less width.
  CHECK(std::format("{:4}|{:4}|{:4}|{:6}", 1, 'c', "s", true) == "   1|c   |s   |true  ");
  CHECK(std::format("{:4d}|{:4x}", 'c', true) == "  99|   1");  // an integer presentation type -> right
  // 0 with an explicit alignment and a multi-code-unit fill: no zeros (/8).
  CHECK(std::format("{:é<06}", -42) == "-42ééé");
  CHECK(std::format("{:é>06}", 42) == "éééé42");
  CHECK(std::format("{:06}", -42) == "-00042");
  // Fills that look like syntax: an alignment character, a digit, '#', '+', '.', 'L', ':'.
  CHECK(std::format("{:<>3}", 1) == "<<1");
  CHECK(std::format("{:1^5}", 0) == "11011");
  CHECK(std::format("{:#>3}", 1) == "##1");
  CHECK(std::format("{:+<3}", 1) == "1++");
  CHECK(std::format("{:.^5}", "x") == "..x..");
  CHECK(std::format("{:L>3}", 1) == "LL1");
  CHECK(std::format("{::>4}", 7) == ":::7");   // ':' is a valid fill of a std-format-spec
  // Ranges and tuples: a multi-code-unit fill for the whole range / tuple and in the
  // range-underlying-spec.
  const std::vector<int> v = {1, 2};
  CHECK(std::format("{:é^11}", v) == "éé[1, 2]ééé");
  CHECK(std::format("{:é<9n}", v) == "1, 2ééééé");
  CHECK(std::format("{::é>3}", v) == "[éé1, éé2]");
  CHECK(std::format("{:*>12:é<3}", v) == "**[1éé, 2éé]");
  CHECK(std::format("{:é>8}", std::pair(1, 2)) == "éé(1, 2)");
  CHECK(std::format("{:🤡^10}", std::tuple(1, 2)) == "🤡🤡(1, 2)🤡🤡");
  // A range-fill cannot be ':' ([format.range.formatter]/2): "{::>4}" is an empty
  // range-format-spec followed by the range-underlying-spec ">4".
  CHECK(std::format("{::>4}", v) == "[   1,    2]");
  CHECK(std::format("{:::>4}", v) == "[:::1, :::2]");  // underlying fill ':'
  // Wide: the fill is one UTF-32 code unit, whatever its width.
  CHECK(std::format(L"{:中^6}", L"ab") == L"中中ab中中");
  CHECK(std::format(L"{:\U0001F921>3}", 1) == L"\U0001F921\U0001F9211");
  CHECK(std::format(L"{:é<4}", L'x') == L"xééé");
  CHECK(std::format(L"{::é>3}", std::vector<int>{5}) == L"[éé5]");
  return 0;
}
