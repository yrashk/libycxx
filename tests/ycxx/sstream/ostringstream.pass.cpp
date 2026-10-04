// [ostringstream]: basic_ostringstream writes into its stringbuf (out is always set); the
// initial string is overwritten from the beginning unless ate is given ([stringbuf.members]/3);
// str() / view() / str() &&.
#include <sstream>
#include <string>
#include <utility>
#include "check.hpp"

int main() {
  std::ostringstream os;
  os << "a" << 1 << ' ' << 2.5 << ' ' << true << ' ' << 'c';
  CHECK(os.str() == "a1 2.5 1 c");
  CHECK(os.view() == os.str());

  std::ostringstream over("hello");
  over << "HE";
  CHECK(over.str() == "HEllo");

  std::ostringstream ate("hello", std::ios_base::ate);
  ate << " world";
  CHECK(ate.str() == "hello world");

  std::string out = std::move(os).str();
  CHECK(out == "a1 2.5 1 c");
  CHECK(os.str().empty());

  std::ostringstream w;
  w.width(5);
  w << 42;
  CHECK(w.str() == "   42");
  std::wostringstream ws;
  ws << L"wide " << 7;
  CHECK(ws.str() == L"wide 7");
  return 0;
}
