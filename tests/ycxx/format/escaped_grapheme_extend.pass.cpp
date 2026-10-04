// [format.string.escaped]/2 (UTF-8 / UTF-32 encodings): for each code unit sequence X of S,
// (2.2.1.1) Table 114 characters become two-character escapes; (2.2.1.2) otherwise, a C
// other than U+0020 is written as \u{hex} if (2.2.1.2.1) its General_Category is in the
// groups Separator (Z) or Other (C), or (2.2.1.2.2) it has Grapheme_Extend=Yes "and C is not
// immediately preceded in S by a character P appended to E without translation to an escape
// sequence"; (2.2.3) each code unit of an ill-formed sequence becomes \x{hex}. /3: escaped
// characters use apostrophes, escape ' and leave " unchanged. Example 1, s6 (emoji ZWJ
// sequence: only the ZWJ, category Cf, is escaped; VS16 extends U+2642, which was appended
// untranslated). So a Grapheme_Extend character is escaped at the start, and after a
// character written as \t \n \" \\ \u{...} or after ill-formed code units, but not after a
// space or another untranslated character. Grapheme_Extend is not "every mark": U+0903
// DEVANAGARI SIGN VISARGA (Mc) has Grapheme_Extend=No, U+FF9E HALFWIDTH KATAKANA VOICED SOUND
// MARK (Lm) has Grapheme_Extend=Yes (Other_Grapheme_Extend, PropList.txt).
// Debug format of range and tuple elements: [format.range.formatter]/9, [format.tuple]/7.
#include <format>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "check.hpp"

int main() {
  // Example 1, s6.
  CHECK(std::format("[{:?}]", "\U0001F937\U0001F3FB\u200d\u2642\ufe0f") ==
        "[\"\U0001F937\U0001F3FB\\u{200d}\u2642\ufe0f\"]");
  // Grapheme_Extend after a translated character.
  CHECK(std::format("{:?}", "\"\u0301") == "\"\\\"\\u{301}\"");
  CHECK(std::format("{:?}", "\n\u0301") == "\"\\n\\u{301}\"");
  CHECK(std::format("{:?}", "\\\u0301") == "\"\\\\\\u{301}\"");
  CHECK(std::format("{:?}", "\u200b\u0301") == "\"\\u{200b}\\u{301}\"");
  CHECK(std::format("{:?}", "\u0301\u0301") == "\"\\u{301}\\u{301}\"");
  CHECK(std::format("{:?}", "\xff" "\u0301") == "\"\\x{ff}\\u{301}\"");
  // ... and after untranslated characters (including the space and ').
  CHECK(std::format("{:?}", " \u0301") == "\" \u0301\"");
  CHECK(std::format("{:?}", "'\u0301") == "\"'\u0301\"");
  CHECK(std::format("{:?}", "a\u0301\u0301") == "\"a\u0301\u0301\"");
  CHECK(std::format("{:?}", "\u4e2d\u3099") == "\"\u4e2d\u3099\"");
  // Grapheme_Extend versus marks.
  CHECK(std::format("{:?}", "\u0903") == "\"\u0903\"");
  CHECK(std::format("{:?}", "\uff9e") == "\"\\u{ff9e}\"");
  CHECK(std::format("{:?}", "\uff76\uff9e") == "\"\uff76\uff9e\"");
  // Z and C categories: Zs (other than U+0020), Zl, Zp, Cc, Cf, Co, Cn.
  CHECK(std::format("{:?}", "\u3000") == "\"\\u{3000}\"");
  CHECK(std::format("{:?}", "\u2029") == "\"\\u{2029}\"");
  CHECK(std::format("{:?}", "\u0085") == "\"\\u{85}\"");
  CHECK(std::format("{:?}", "\u00ad") == "\"\\u{ad}\"");
  CHECK(std::format("{:?}", "\ufeff") == "\"\\u{feff}\"");
  CHECK(std::format("{:?}", "\ue000") == "\"\\u{e000}\"");
  CHECK(std::format("{:?}", "\u0378") == "\"\\u{378}\"");
  CHECK(std::format("{:?}", "\U000E0001") == "\"\\u{e0001}\"");  // LANGUAGE TAG, Cf
  // Ill-formed UTF-8: every code unit of the ill-formed sequence.
  CHECK(std::format("{:?}", "\xed\xa0\x80") == "\"\\x{ed}\\x{a0}\\x{80}\"");      // surrogate
  CHECK(std::format("{:?}", "\xc0\x80") == "\"\\x{c0}\\x{80}\"");                 // overlong
  CHECK(std::format("{:?}", "\xf4\x90\x80\x80") == "\"\\x{f4}\\x{90}\\x{80}\\x{80}\"");  // > U+10FFFF
  CHECK(std::format("{:?}", "\xf0\x9f\x98" "!") == "\"\\x{f0}\\x{9f}\\x{98}!\"");  // truncated
  CHECK(std::format("{:?}", "a\x80" "b") == "\"a\\x{80}b\"");                     // stray continuation
  CHECK(std::format("{:?}", "\xe4\xb8\xad\xe4") == "\"\u4e2d\\x{e4}\"");
  // Characters.
  CHECK(std::format("{:?}", '\0') == "'\\u{0}'");
  CHECK(std::format("{:?}", '\t') == "'\\t'");
  CHECK(std::format("{:?}", '\r') == "'\\r'");
  CHECK(std::format("{:?}", '"') == "'\"'");
  CHECK(std::format("{:?}", '\'') == "'\\''");
  CHECK(std::format("{:?}", static_cast<char>(0xc3)) == "'\\x{c3}'");
  CHECK(std::format("{:?}", "'") == "\"'\"");
  // UTF-32 (wchar_t): code units that are not scalar values are ill-formed.
  CHECK(std::format(L"{:?}", L"\xd800") == L"\"\\x{d800}\"");
  CHECK(std::format(L"{:?}", L"a\x110000" L"b") == L"\"a\\x{110000}b\"");
  CHECK(std::format(L"{:?}", static_cast<wchar_t>(0xdfff)) == L"'\\x{dfff}'");
  CHECK(std::format(L"{:?}", L'\u0301') == L"'\\u{301}'");
  CHECK(std::format(L"{:?}", L"a\u0301") == L"\"a\u0301\"");
  CHECK(std::format(L"{:?}", L"\"\u0301") == L"\"\\\"\\u{301}\"");
  CHECK(std::format(L"{:?}", L"\U0001F937\U0001F3FB\u200d\u2642\ufe0f") ==
        L"\"\U0001F937\U0001F3FB\\u{200d}\u2642\ufe0f\"");
  CHECK(std::format(L"{:?}", L'"') == L"'\"'");
  // Debug format of range and tuple elements.
  CHECK(std::format("{}", std::vector<std::string>{"\u0301", "a\u0301"}) == "[\"\\u{301}\", \"a\u0301\"]");
  CHECK(std::format("{}", std::pair('\n', "\xff")) == "('\\n', \"\\x{ff}\")");
  CHECK(std::format("{}", std::vector<char>{'\'', '"'}) == "['\\'', '\"']");
  CHECK(std::format("{:?s}", std::vector<char>{'\'', '"'}) == "\"'\\\"\"");
  CHECK(std::format(L"{}", std::tuple(L"\u200b", L'\\')) == L"(\"\\u{200b}\", '\\\\')");
  return 0;
}
