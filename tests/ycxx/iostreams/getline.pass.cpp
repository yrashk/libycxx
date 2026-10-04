// [string.io]: getline(is, str, delim): extracts characters until end-of-file (eofbit), the
// delimiter (extracted and discarded), or str.max_size() characters; "If the function extracts
// no characters, it calls is.setstate(ios_base::failbit)"; the string is erased after a successful
// sentry.
// [istream.unformatted]: the member getline(s, n, delim): stores up to n - 1 characters and a
// null; failbit if n - 1 characters were stored without reaching the delimiter, or nothing was
// extracted; gcount() counts the delimiter too.
#include <sstream>
#include <string>
#include <cstring>
#include "check.hpp"

int main() {
  std::istringstream is("first line\nsecond;third\n\nlast");
  std::string s = "old";
  std::getline(is, s);
  CHECK(s == "first line" && is.good());
  std::getline(is, s, ';');
  CHECK(s == "second");
  std::getline(is, s);
  CHECK(s == "third");
  std::getline(is, s);
  CHECK(s.empty() && is.good());  // an empty line: the delimiter was extracted
  std::getline(is, s);
  CHECK(s == "last" && is.eof() && !is.fail());
  std::getline(is, s);  // eofbit set: the sentry fails, str is not erased, failbit is set
  CHECK(s == "last" && is.fail());
  std::istringstream nothing("x");
  nothing.get();
  std::getline(nothing, s);  // sentry fine, no character extracted
  CHECK(s.empty() && nothing.fail() && nothing.eof());

  std::istringstream rv("a,b");
  std::string t;
  std::getline(std::move(rv), t, ',');  // rvalue stream overload
  CHECK(t == "a");

  std::wistringstream w(L"wide\nx");
  std::wstring ws;
  std::getline(w, ws);
  CHECK(ws == L"wide");

  // member getline
  std::istringstream m("abcdef\nxy\n");
  char buf[4];
  m.getline(buf, 4);
  CHECK(std::strcmp(buf, "abc") == 0);
  CHECK(m.fail() && m.gcount() == 3);  // 3 stored, no delimiter
  m.clear();
  m.getline(buf, 4);
  CHECK(std::strcmp(buf, "def") == 0 && m.good() && m.gcount() == 4);  // "def" + '\n' (delimiter fits exactly)
  m.getline(buf, 4, 'y');
  CHECK(std::strcmp(buf, "x") == 0 && m.gcount() == 2);
  m.getline(buf, 4);
  CHECK(buf[0] == '\0' && m.gcount() == 1 && m.good());
  m.getline(buf, 4);
  CHECK(buf[0] == '\0' && m.fail() && m.eof() && m.gcount() == 0);
  return 0;
}
