// [quoted.manip]/2: out << quoted(s, delim, escape) writes delim, each character of s preceded
// by escape when it equals escape or delim, then delim; the whole sequence is padded to
// width() as a unit and width(0) is called. /3: in >> quoted(s): if the first character is
// delim, turns skipws off, clears s, reads until an unescaped delim (dropping escapes),
// discards the closing delim, restores skipws; "Otherwise, in >> s." Round trips preserve
// embedded spaces.
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include "check.hpp"

int main() {
  std::ostringstream os;
  os << std::quoted("a \"b\" \\c");
  CHECK(os.str() == "\"a \\\"b\\\" \\\\c\"");

  std::ostringstream custom;
  custom << std::quoted(std::string("x'y"), '\'', '&') << std::quoted(std::string_view("p&q"), '\'', '&');
  CHECK(custom.str() == "'x&'y''p&&q'");

  std::ostringstream pad;
  pad << std::left << std::setw(8) << std::setfill('.') << std::quoted("ab") << 1;
  CHECK(pad.str() == "\"ab\"....1");  // padded as a unit, width reset afterwards

  std::istringstream is("\"hello world\" next \"esc\\\"aped\" plain");
  std::string a = "x", b, c, d;
  is >> std::quoted(a) >> b >> std::quoted(c) >> std::quoted(d);
  CHECK(a == "hello world");
  CHECK(b == "next");
  CHECK(c == "esc\"aped");
  CHECK(d == "plain");  // not starting with delim: in >> s
  CHECK(is.flags() & std::ios_base::skipws);  // restored

  // round trip
  const std::string orig = "  spaces\tand \"quotes\" \\ ";
  std::stringstream ss;
  ss << std::quoted(orig);
  std::string back;
  ss >> std::quoted(back);
  CHECK(back == orig);

  // an unterminated quote reads to the end
  std::istringstream un("\"open ended");
  std::string u;
  un >> std::quoted(u);
  CHECK(u == "open ended" && un.eof());

  std::wostringstream w;
  w << std::quoted(L"w\"");
  CHECK(w.str() == L"\"w\\\"\"");
  return 0;
}
