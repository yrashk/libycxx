// Formatting with charT = wchar_t ([format.functions]/4, /10: the wformat_string overloads;
// [format.formatter.spec]/2: the same enabled specializations for each charT).
// formatter<char, wchar_t> (/2.1, debug-enabled): [format.arg]/6.2 stores a char argument of
// a wide context as static_cast<wchar_t>(static_cast<unsigned char>(v)), so '\x80' formats
// as U+0080 (escaped as \u{80}: General_Category Cc, [format.string.escaped]/2.2.1.2.1) and
// the integer presentations see 0..255. [format.string.std]/20: to_chars output is
// transcoded to the wide literal encoding when charT is wchar_t; Tables 106-111 apply
// unchanged; [format.string.std]/13: width in UTF-32. String types ([format.formatter.spec]
// /2.2): wchar_t*, const wchar_t*, wchar_t[N], wstring, wstring_view. Ranges and tuples
// ([format.range], [format.tuple]) with wide brackets and separators.
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include "check.hpp"

int main() {
  // formatter<char, wchar_t>
  CHECK(std::format(L"{}", 'a') == L"a");
  CHECK(std::format(L"{:c}|{:*^5}", 'a', 'x') == L"a|**x**");
  CHECK(std::format(L"{:?}", 'a') == L"'a'");
  CHECK(std::format(L"{:?}", '\n') == L"'\\n'");
  CHECK(std::format(L"{:d} {:#x} {:b}", 'A', 'A', 'A') == L"65 0x41 1000001");
  CHECK(std::format(L"{:d}", static_cast<char>(0x80)) == L"128");
  CHECK(std::format(L"{}", static_cast<char>(0xe9)) == L"é");
  CHECK(std::format(L"{:?}", static_cast<char>(0x80)) == L"'\\u{80}'");
  CHECK(std::format(L"{:>4d}", 'z') == L" 122");
  // wchar_t
  CHECK(std::format(L"{} {:?} {:x}", L'中', L'\t', L'中') == L"中 '\\t' 4e2d");
  CHECK(std::format(L"{:3}|", L'中') == L"中 |");   // width 2
  // Integers, bool, floating point, pointers: to_chars output in wide characters.
  CHECK(std::format(L"{:#X} {:+d} {:o} {:#B}", 255, 7, 8, 5) == L"0XFF +7 10 0B101");
  CHECK(std::format(L"{}", std::numeric_limits<long long>::min()) == L"-9223372036854775808");
  CHECK(std::format(L"{:06}|{:<6}|{:^7}", -42, 42, 42) == L"-00042|42    |  42   ");
  CHECK(std::format(L"{} {:s} {:d} {:6}|", true, false, true, true) == L"true false 1 true  |");
  CHECK(std::format(L"{:+.3e} {:a} {:.2f} {:G}", 1.5, 1.0, 2.675, 1e-10) == L"+1.500e+00 1p+0 2.67 1E-10");
  CHECK(std::format(L"{} {:F}", -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()) ==
        L"-inf NAN");
  CHECK(std::format(L"{}", 0.1f) == L"0.1");
  CHECK(std::format(L"{:P} {:010}", reinterpret_cast<void*>(0xabc), nullptr) == L"0XABC 0x00000000");
  // String types.
  wchar_t buf[] = L"arr";
  wchar_t* mp = buf;
  const wchar_t* cp = L"cp";
  CHECK(std::format(L"{}|{}|{}|{}|{}", buf, mp, cp, std::wstring(L"ws"), std::wstring_view(L"wv")) ==
        L"arr|arr|cp|ws|wv");
  CHECK(std::format(L"{:.2}|{:>5.1}|{:s}", L"wide", std::wstring(L"xyz"), cp) == L"wi|    x|cp");
  CHECK(std::format(L"{:.3}|", L"中文") == L"中|");
  CHECK(std::format(L"{:?}", std::wstring_view(L"a\0b", 3)) == L"\"a\\u{0}b\"");
  // Ranges and tuples.
  CHECK(std::format(L"{}", std::vector<wchar_t>{L'a', L'\''}) == L"['a', '\\'']");
  CHECK(std::format(L"{:s}", std::vector<wchar_t>{L'a', L'b'}) == L"ab");
  CHECK(std::format(L"{}", std::vector<char>{'a'}) == L"['a']");          // formatter<char, wchar_t>
  CHECK(std::format(L"{}", std::pair('c', L"s")) == L"('c', \"s\")");
  CHECK(std::format(L"{:m}", std::tuple(1, 2.5)) == L"1: 2.5");
  CHECK(std::format(L"{:*^10}", std::vector<int>{1, 2}) == L"**[1, 2]**");
  CHECK(std::format(L"{::#x}", std::vector<unsigned>{10, 11}) == L"[0xa, 0xb]");
  std::range_formatter<int, wchar_t> rf;
  rf.set_separator(L"; ");
  rf.set_brackets(L"<", L">");
  std::formatter<std::pair<int, int>, wchar_t> pf;
  pf.set_separator(L"-");
  pf.set_brackets(L"<", L">");
  // Escapes in format strings.
  CHECK(std::format(L"{{{}}}", 1) == L"{1}");
  return 0;
}
