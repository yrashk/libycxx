// [string.syn] declares operator<<, operator>> and the four getline overloads of basic_string in
// <string>, and [string.view.synop] declares operator<< of basic_string_view in <string_view>;
// [iosfwd.syn] declares the stream types. A translation unit that includes only those headers
// (support/linkage/string_io_tu2.cpp) may use them on streams it is given by reference
// ([using.headers]/1-2: including the header that declares an entity makes it available), and
// the program then behaves as [string.io] and [string.view.io] specify, whether or not another
// translation unit includes the stream headers.
// FILES: ../support/linkage/string_io_tu2.cpp
#include <sstream>
#include <string>
#include <string_view>
#include "check.hpp"

void tu2_put(std::ostream& os, const std::string& s, std::string_view v);
void tu2_wput(std::wostream& os, const std::wstring& s, std::wstring_view v);
void tu2_get(std::istream& is, std::string& word, std::string& rest, std::string& field);
void tu2_wget(std::wistream& is, std::wstring& word, std::wstring& rest, std::wstring& field);
void tu2_get_rvalue(std::istream&& is, std::string& line, std::string& field);

int main() {
  std::ostringstream o;
  tu2_put(o, "ab", "cd");
  CHECK(o.str() == "abcd");
  std::wostringstream wo;
  tu2_wput(wo, L"ab", L"cd");
  CHECK(wo.str() == L"abcd");

  std::istringstream i("word rest of line\nx;y");
  std::string word, rest, field;
  tu2_get(i, word, rest, field);
  CHECK(word == "word");
  CHECK(rest == " rest of line");
  CHECK(field == "x");

  std::wistringstream wi(L"word rest\nx;y");
  std::wstring wword, wrest, wfield;
  tu2_wget(wi, wword, wrest, wfield);
  CHECK(wword == L"word" && wrest == L" rest" && wfield == L"x");

  std::string line, f;
  tu2_get_rvalue(std::istringstream("a;b c"), line, f);
  CHECK(f == "a" && line == "b c");
  return 0;
}
