// [re.grammar]: the ECMAScript grammar of ECMA-262 (with POSIX class names added): character
// classes \d \w \s and their negations (equivalent to [[:digit:]], [_[:alnum:]], [[:space:]]),
// bracket expressions with ranges, negation and [:name:] classes, anchors ^ $ and word
// boundaries \b \B, greedy and non-greedy quantifiers * + ? {n} {n,} {n,m}, grouping,
// non-capturing groups (?:), lookahead (?=) (?!), backreferences \n, escapes \t \n \xhh \uhhhh
// \0 and \cX, and the dot.
// COUNTERPART: libcxx:re/re.alg/re.alg.match/ecma.pass.cpp
#include <regex>
#include <string>
#include "check.hpp"

bool full(const char* re, const char* s) { return std::regex_match(s, std::regex(re)); }
std::string first(const char* re, const char* s, int g = 0) {
  std::cmatch m;
  return std::regex_search(s, m, std::regex(re)) ? m.str(g) : std::string("<none>");
}

int main() {
  CHECK(full("\\d\\d", "42") && !full("\\d", "a") && full("\\D+", "ab"));
  CHECK(full("\\w+", "a_Z9") && !full("\\w", "-") && full("\\W", "-"));
  CHECK(full("\\s+", " \t\n") && full("\\S", "x"));
  CHECK(full("[a-c]+", "abcab") && !full("[a-c]", "d") && full("[^a-c]", "d"));
  CHECK(full("[[:digit:][:upper:]]+", "A1B2") && !full("[[:digit:]]", "a"));
  CHECK(full("[[:alpha:]]+", "Abc") && full("[[:space:]]", " ") && full("[[:xdigit:]]+", "09afAF"));
  CHECK(full("[[:alnum:]_]+", "a_1") && full("[[:punct:]]", "!") && full("[[:lower:]]+", "abc"));
  CHECK(full("[\\d-]+", "1-2") && full("[a\\]]+", "a]"));

  CHECK(first("^ab", "abab") == "ab" && first("ab$", "abab") == "ab" && first("^b", "ab") == "<none>");
  CHECK(first("\\bcat\\b", "concat cat") == "cat");
  std::cmatch m;
  CHECK(std::regex_search("concat cat", m, std::regex("\\bcat\\b")) && m.position() == 7);
  CHECK(first("\\Bcat", "concat cat") == "cat");

  CHECK(first("a*", "aaab") == "aaa" && first("a+?", "aaa") == "a" && first("a*?b", "aaab") == "aaab");
  CHECK(first("a{2}", "aaaa") == "aa" && first("a{2,}", "aaaa") == "aaaa" && first("a{1,3}", "aaaa") == "aaa");
  CHECK(first("a{1,3}?", "aaaa") == "a" && first("ab?c", "ac") == "ac" && first("ab??", "ab") == "a");
  CHECK(first("<.*>", "<a><b>") == "<a><b>" && first("<.*?>", "<a><b>") == "<a>");

  CHECK(first("(a)(?:b)(c)", "abc", 2) == "c");
  CHECK(std::regex("(a)(?:b)(c)").mark_count() == 2);
  CHECK(first("foo(?=bar)", "foobaz foobar") == "foo");
  CHECK(std::regex_search("foobaz foobar", m, std::regex("foo(?=bar)")) && m.position() == 7);
  CHECK(std::regex_search("foobar foobaz", m, std::regex("foo(?!bar)")) && m.position() == 7);
  CHECK(full("(\\w)\\1", "aa") && !full("(\\w)\\1", "ab"));
  CHECK(full("(a)(b)\\2\\1", "abba"));

  CHECK(full("\\t", "\t") && full("\\n", "\n") && full("\\x41", "A") && full("\\u0042", "B"));
  CHECK(full("\\cJ", "\n") && full("a\\.b", "a.b") && !full("a\\.b", "axb"));
  CHECK(full("a.c", "abc") && full("a.c", "a-c"));
  CHECK(full("(a|b|c)+", "abcba") && !full("(a|b)+", "abc"));
  CHECK(full("(?:ab)*", "") && full("(?:ab)*", "abab"));
  // ECMAScript backtracking semantics: the capture is from the last iteration.
  CHECK(first("(a|b)*", "abab", 1) == "b");
  return 0;
}
