// [format.string.std]/13: "For a sequence of characters in UTF-8, UTF-16, or UTF-32, an
// implementation should use as its field width the sum of the field widths of the first code
// point of each extended grapheme cluster. Extended grapheme clusters are defined by UAX #29
// of the Unicode Standard. The following code points have a field width of 2: (13.1) any code
// point with the East_Asian_Width="W" or East_Asian_Width="F" property as described by UAX
// #44 ... (13.2) U+4dc0 - U+4dff ... (13.3) U+1f300 - U+1f5ff ... (13.4) U+1f900 - U+1f9ff.
// The field width of all other code points is 1." /15: for strings, precision "specifies the
// longest prefix of the formatted argument to be included in the replacement field such that
// the field width of the prefix is no greater than the value of this option."
// Clusters per UAX #29 (Table 2, rules GB3-GB13): CR LF (GB3); Hangul L V T and LV T
// syllable sequences (GB6-GB8); Extend and ZWJ (GB9, e.g. combining marks, VS16, the emoji
// modifiers U+1F3FB-U+1F3FF whose Grapheme_Cluster_Break is Extend); SpacingMark (GB9a,
// U+0903); Prepend (GB9b, U+0600); emoji ZWJ sequences (GB11); regional indicator pairs
// (GB12-GB13). Only the cluster's first code point counts, so a wide modifier after a narrow
// base adds nothing, and a cluster of several wide code points counts 2.
// East_Asian_Width values from UAX #11 / EastAsianWidth.txt: U+1F1E6-U+1F1FF (regional
// indicators) N, U+261D N, U+2764 N, U+1F321 N, U+1F900 N, U+1F650 N, U+1FA00 N, U+303F N,
// U+1100 W, U+1161 N, U+11A8 N, U+3000 F, U+231A W, U+20000 W.
// (The literal encoding is UTF-8; wchar_t is UTF-32 on this platform.)
#include <format>
#include <string>
#include <string_view>
#include "check.hpp"

// Width as observed through padding: the number of fill characters added to reach `w`.
static std::size_t pad(std::string_view s, int w) {
  std::string out = std::vformat("{:*<{}}", std::make_format_args(s, w));
  std::size_t n = 0;
  for (std::size_t i = out.size(); i > 0 && out[i - 1] == '*'; --i) ++n;
  return n;
}

int main() {
  // GB3: CR LF is one cluster (width 1); other controls are clusters of their own.
  CHECK(pad("a\r\nb", 5) == 2);
  CHECK(pad("\n\r", 5) == 3);
  // GB6-GB8: conjoining jamo L V T and LV T form one cluster whose first code point is W.
  CHECK(pad("각", 4) == 2);
  CHECK(pad("각", 4) == 2);
  CHECK(pad("ᅡᆨ", 4) == 3);   // V T: one cluster starting with an N code point
  // GB9: Extend and ZWJ do not start clusters. A wide Extend code point after a narrow base
  // does not count (U+1F3FB is W but extends U+261D, which is N).
  CHECK(pad("☝\U0001F3FB", 3) == 2);
  CHECK(pad("\U0001F937\U0001F3FB", 3) == 1);   // U+1F937 (in U+1F900-U+1F9FF) + modifier: 2
  CHECK(pad("❤️", 3) == 2);           // HEAVY BLACK HEART is N even with VS16
  CHECK(pad("a゙", 3) == 2);                // a W combining mark after a narrow base
  // GB9a / GB9b: SpacingMark joins the preceding cluster, Prepend the following one.
  CHECK(pad("कः", 3) == 2);
  CHECK(pad("؀١", 3) == 2);
  // GB11: an emoji ZWJ sequence is one cluster of width 2.
  CHECK(pad("\U0001F468‍\U0001F469‍\U0001F467", 4) == 2);
  CHECK(pad("\U0001F937\U0001F3FB‍♂️", 4) == 2);
  // GB12-GB13: regional indicators pair up; each pair is one cluster of width 1.
  CHECK(pad("\U0001F1FA\U0001F1F8", 3) == 2);
  CHECK(pad("\U0001F1FA\U0001F1F8\U0001F1EB", 3) == 1);
  CHECK(pad("\U0001F1FA\U0001F1F8\U0001F1EB\U0001F1F7", 3) == 1);
  // The explicitly listed ranges apply whatever East_Asian_Width says, and only there.
  CHECK(pad("\U0001F321", 3) == 1);   // N, inside U+1F300-U+1F5FF
  CHECK(pad("\U0001F900", 3) == 1);   // N, first of U+1F900-U+1F9FF
  CHECK(pad("䷿", 3) == 1);       // last of U+4DC0-U+4DFF
  CHECK(pad("\U0001F650", 3) == 2);   // N, between the two pictograph ranges
  CHECK(pad("\U0001FA00", 3) == 2);   // N, just after U+1F9FF
  // East_Asian_Width W / F outside the listed ranges; N inside a CJK block.
  CHECK(pad("　", 3) == 1);
  CHECK(pad("⌚", 3) == 1);
  CHECK(pad("\U00020000", 3) == 1);
  CHECK(pad("〿", 3) == 2);
  // Precision (/15): the longest prefix whose own width does not exceed the precision; a
  // cluster's trailing code points add no width, so they are kept with their first one.
  CHECK(std::format("{:.2}|", "각x") == "각|");
  CHECK(std::format("{:.1}|", "각x") == "|");
  CHECK(std::format("{:.1}|", "☝\U0001F3FBx") == "☝\U0001F3FB|");
  CHECK(std::format("{:.2}|", "\U0001F468‍\U0001F469‍\U0001F467x") ==
        "\U0001F468‍\U0001F469‍\U0001F467|");
  CHECK(std::format("{:.1}|", "\U0001F1FA\U0001F1F8\U0001F1EB\U0001F1F7") == "\U0001F1FA\U0001F1F8|");
  CHECK(std::format("{:.1}|", "a\r\nb") == "a|");
  CHECK(std::format("{:.2}|", "a\r\nb") == "a\r\n|");
  CHECK(std::format("{:.0}|", "؀١") == "|");
  // Precision and width together.
  CHECK(std::format("{:*<4.2}|", "\U0001F1FA\U0001F1F8\U0001F1EB\U0001F1F7x") ==
        "\U0001F1FA\U0001F1F8\U0001F1EB\U0001F1F7**|");
  // The same rules for wide strings (UTF-32).
  CHECK(std::format(L"{:*<4}|", L"각") == L"각**|");
  CHECK(std::format(L"{:*<3}|", L"☝\U0001F3FB") == L"☝\U0001F3FB**|");
  CHECK(std::format(L"{:*<3}|", L"\U0001F1FA\U0001F1F8\U0001F1EB") == L"\U0001F1FA\U0001F1F8\U0001F1EB*|");
  CHECK(std::format(L"{:*<4}|", L"\U0001F468‍\U0001F469") == L"\U0001F468‍\U0001F469**|");
  CHECK(std::format(L"{:.1}|", L"\U0001F1FA\U0001F1F8\U0001F1EB") == L"\U0001F1FA\U0001F1F8|");
  CHECK(std::format(L"{:*<3}|", L"\U0001F650") == L"\U0001F650**|");
  return 0;
}
