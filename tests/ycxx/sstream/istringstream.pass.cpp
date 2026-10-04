// [istringstream]: basic_istringstream reads from its stringbuf (opened with in); formatted
// extraction skips white space, sets eofbit when the end is reached while extracting, and
// failbit when nothing can be extracted; str() / str(s) / view(); rdbuf() is the member
// stringbuf; move construction and swap transfer the buffer.
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include "check.hpp"

static_assert(std::is_same_v<std::istringstream, std::basic_istringstream<char>>);
static_assert(std::is_base_of_v<std::istream, std::istringstream>);
static_assert(!std::is_copy_constructible_v<std::istringstream>);
static_assert(std::is_move_constructible_v<std::istringstream>);

int main() {
  std::istringstream is("  42 word 3.5\n-7");
  int i = 0;
  std::string w;
  double d = 0;
  int n = 0;
  is >> i >> w >> d >> n;
  CHECK(i == 42 && w == "word" && d == 3.5 && n == -7);
  CHECK(is.eof() && !is.fail());
  is >> n;  // eofbit already set: the sentry fails ([istream.sentry]/2), n is untouched
  CHECK(is.fail() && is.eof());
  CHECK(n == -7);

  std::istringstream bad("abc");
  int x = 5;
  bad >> x;
  CHECK(bad.fail() && !bad.eof());
  CHECK(x == 0);  // [facet.num.get.virtuals]: zero is stored when the conversion fails
  bad.clear();
  bad >> w;
  CHECK(w == "abc");

  std::istringstream s;
  s.str("1 2");
  CHECK(s.view() == "1 2");
  int a, b;
  s >> a >> b;
  CHECK(a == 1 && b == 2);
  CHECK(s.rdbuf()->str() == "1 2");

  std::istringstream m(std::move(s));
  CHECK(m.str() == "1 2");
  std::istringstream t("t");
  m.swap(t);
  CHECK(m.str() == "t" && t.str() == "1 2");
  std::istringstream wide_mode("q", std::ios_base::out);  // in is always added
  char c;
  wide_mode >> c;
  CHECK(c == 'q');
  return 0;
}
