// [format.string.std]/3, /13 and /15 (UTF-8 literals): the fill character is one Unicode
// scalar value; the field width of a string is the sum of the widths of the first code point
// of each extended grapheme cluster, 2 for East_Asian_Width W or F and for U+4DC0-U+4DFF,
// U+1F300-U+1F5FF, U+1F900-U+1F9FF, 1 otherwise; precision keeps the longest prefix whose
// width does not exceed it. Includes the 🤡 examples of /4.
#include <format>
#include <string>
#include "check.hpp"

int main() {
  // Examples sB and sC of [format.string.std]/4.
  CHECK(std::format("{:🤡^6}", "x") == "🤡🤡x🤡🤡🤡");
  CHECK(std::format("{:*^6}", "🤡🤡🤡") == "🤡🤡🤡");
  CHECK(std::format("{:*<4}|", "🤡") == "🤡**|");
  // East Asian wide and fullwidth characters.
  CHECK(std::format("{:4}|", "中") == "中  |");
  CHECK(std::format("{:>5}|", "中文") == " 中文|");
  CHECK(std::format("{:3}|", "\uff21") == "\uff21 |");   // FULLWIDTH LATIN CAPITAL LETTER A (F)
  CHECK(std::format("{:3}|", "\uac00") == "\uac00 |");   // HANGUL SYLLABLE GA (W)
  // The explicitly listed ranges.
  CHECK(std::format("{:3}|", "\u4dc0") == "\u4dc0 |");   // HEXAGRAM FOR THE CREATIVE HEAVEN
  CHECK(std::format("{:3}|", "\U0001F300") == "\U0001F300 |"); // CYCLONE
  CHECK(std::format("{:3}|", "\U0001F9FF") == "\U0001F9FF |"); // NAZAR AMULET
  // Width 1: Latin, Cyrillic, Greek, and narrow symbols.
  CHECK(std::format("{:3}|", "\u00e9") == "\u00e9  |");
  CHECK(std::format("{:4}|", "\u0416\u03a9") == "\u0416\u03a9  |");
  CHECK(std::format("{:3}|", "\u2665") == "\u2665  |"); // BLACK HEART SUIT (A, ambiguous -> 1)
  // A grapheme cluster counts only its first code point.
  CHECK(std::format("{:3}|", "e\u0301") == "e\u0301  |");
  CHECK(std::format("{:4}|", "e\u0301\u0323x") == "e\u0301\u0323x  |");
  // Precision: the longest prefix with width <= precision.
  CHECK(std::format("{:.1}|", "中x") == "|");
  CHECK(std::format("{:.2}|", "中x") == "中|");
  CHECK(std::format("{:.3}|", "中x") == "中x|");
  CHECK(std::format("{:.1}|", "e\u0301x") == "e\u0301|");
  CHECK(std::format("{:.3}|", "🤡🤡") == "🤡|");
  CHECK(std::format("{:*<4.3}|", "🤡🤡") == "🤡**|");
  // A non-ASCII fill character.
  CHECK(std::format("{:é>3}", 1) == "éé1");
  CHECK(std::format("{:中<3}", 'a') == "a中中");
  // Wide strings (UTF-32 here).
  CHECK(std::format(L"{:4}|", L"中") == L"中  |");
  CHECK(std::format(L"{:.2}|", L"中x") == L"中|");
}
