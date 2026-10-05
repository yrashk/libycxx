// Regular expressions with a thousand marked subexpressions, side by side and nested 300 deep,
// with back-references to high-numbered groups. The draft sets no limit on their number.
//   [re.regex.operations]/1 mark_count: "Returns the number of marked sub-expressions within
//     the regular expression."
//   [re.results.size]/1-2: size() is "One plus the number of marked sub-expressions in the
//     regular expression that was matched if *this represents the result of a successful match";
//     [re.results.acc] operator[](n), position, length, str for every n < size(); /8 for
//     n >= size() a sub_match that does not match.
//   ECMAScript (ECMA-262 21.2.2.9 DecimalEscape, [re.grammar]): "\1000" is a back-reference to
//     group 1000 when there are at least 1000 groups.
#include <regex>
#include <string>
#include "check.hpp"

int main() {
  constexpr int G = 1000;
  std::string pattern, subject;
  for (int i = 0; i < G; ++i) {
    const char c = static_cast<char>('a' + i % 26);
    pattern += '(';
    pattern += c;
    pattern += "+)";
    subject += std::string(static_cast<std::size_t>(1 + i % 3), c);
  }
  {
    const std::regex re(pattern);
    CHECK(re.mark_count() == G);
    std::smatch m;
    CHECK(std::regex_match(subject, m, re));
    CHECK(m.size() == G + 1);
    std::size_t pos = 0;
    for (int i = 1; i <= G; ++i) {
      const std::size_t len = static_cast<std::size_t>(1 + (i - 1) % 3);
      CHECK(m[i].matched);
      CHECK(static_cast<std::size_t>(m.position(i)) == pos);
      CHECK(static_cast<std::size_t>(m.length(i)) == len);
      pos += len;
    }
    CHECK(!m[G + 1].matched);
    CHECK(!m[G + 5000].matched);
  }
  // A back-reference to group 1000 (which matched one 'l': i = 999, 999 % 26 = 11).
  {
    const std::regex re(pattern + "-\\1000-\\999-\\1");
    CHECK(re.mark_count() == G);
    std::smatch m;
    const std::string g1000(1 + (G - 1) % 3, static_cast<char>('a' + (G - 1) % 26));
    const std::string g999(1 + (G - 2) % 3, static_cast<char>('a' + (G - 2) % 26));
    const std::string s = subject + "-" + g1000 + "-" + g999 + "-a";
    CHECK(std::regex_match(s, m, re));
    CHECK(!std::regex_match(subject + "-" + g999 + "-" + g1000 + "-a", re));
  }
  // Nested 300 deep: ((((x))))
  {
    constexpr int D = 300;
    const std::string nested = std::string(D, '(') + "x" + std::string(D, ')');
    const std::regex re(nested + "y");
    CHECK(re.mark_count() == D);
    std::smatch m;
    const std::string s = "zzxyzz";
    CHECK(std::regex_search(s, m, re));
    CHECK(m.size() == D + 1);
    for (int i = 1; i <= D; ++i) CHECK(m.position(i) == 2 && m.length(i) == 1);
  }
  // regex_replace with groups beyond 9 in the format ($nn: two digits, [re.results.form]
  // with ECMAScript's GetSubstitution).
  {
    std::string p, s;
    for (int i = 0; i < 30; ++i) {
      p += "(" + std::string(1, static_cast<char>('A' + i % 26)) + ")";
      s += static_cast<char>('A' + i % 26);
    }
    CHECK(std::regex_replace(s, std::regex(p), "$30$12$01") == "DLA");
  }
  return 0;
}
