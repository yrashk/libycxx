// [format.string.escaped] (UTF-8 literal encoding): the ? type writes strings in double
// quotes and characters in single quotes, escaping \t \n \r " \\ (Table 114), writing
// Z/C category characters and unpreceded Grapheme_Extend characters as \u{hex}, and
// ill-formed code units as \x{hex}; ' is escaped only in characters and " only in strings.
// Includes the examples of /3.
#include <format>
#include <string>
#include <string_view>
#include "check.hpp"

int main() {
  CHECK(std::format("[{}]", "h\tllo") == "[h\tllo]");
  CHECK(std::format("[{:?}]", "h\tllo") == "[\"h\\tllo\"]");
  CHECK(std::format("[{:?}]", "Спасибо, Виктор ♥!") == "[\"Спасибо, Виктор ♥!\"]");
  CHECK(std::format("[{:?}, {:?}]", '\'', '"') == "['\\'', '\"']");
  CHECK(std::format("[{:?}]", std::string("\0 \n \t \x02 \x1b", 9)) == "[\"\\u{0} \\n \\t \\u{2} \\u{1b}\"]");
  CHECK(std::format("[{:?}]", "\xc3\x28") == "[\"\\x{c3}(\"]");
  CHECK(std::format("[{:?}]", "\u0301") == "[\"\\u{301}\"]");
  CHECK(std::format("[{:?}]", "\\\u0301") == "[\"\\\\\\u{301}\"]");
  CHECK(std::format("[{:?}]", "e\u0301\u0323") == "[\"e\u0301\u0323\"]");
  CHECK(std::format("{:?}", "a\"b\\c\rd\ne") == "\"a\\\"b\\\\c\\rd\\ne\"");
  CHECK(std::format("{:?}", std::string_view("it's")) == "\"it's\"");
  CHECK(std::format("{:?}", '\n') == "'\\n'");
  CHECK(std::format("{:?}", '\\') == "'\\\\'");
  CHECK(std::format("{:?}", 'a') == "'a'");
  CHECK(std::format("{:?}", ' ') == "' '");
  CHECK(std::format("{:?}", '\x7f') == "'\\u{7f}'");          // DEL is Cc
  CHECK(std::format("{:?}", "\u00a0") == "\"\\u{a0}\"");      // NO-BREAK SPACE is Zs
  CHECK(std::format("{:?}", "\u2028") == "\"\\u{2028}\"");    // LINE SEPARATOR is Zl
  CHECK(std::format("{:?}", "\u200b") == "\"\\u{200b}\"");    // ZERO WIDTH SPACE is Cf
  CHECK(std::format("{:?}", "\xff") == "\"\\x{ff}\"");
  CHECK(std::format("{:?}", "\xe4\xb8") == "\"\\x{e4}\\x{b8}\""); // truncated sequence
  CHECK(std::format("{:?}", static_cast<char>('\x80')) == "'\\x{80}'");
  CHECK(std::format("{:?}", std::string("ok")) == "\"ok\"");
  // Width applies to the escaped form.
  CHECK(std::format("{:*<6?}", "a\n") == "\"a\\n\"*");
  CHECK(std::format("{:>5?}", 'x') == "  'x'");
  CHECK(std::format("{:.3?}", "abcdef") == "\"ab");
  // Wide.
  CHECK(std::format(L"{:?}", L"a\tb") == L"\"a\\tb\"");
  CHECK(std::format(L"{:?}", L'\'') == L"'\\''");
}
