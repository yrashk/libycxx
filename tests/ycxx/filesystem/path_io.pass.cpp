// [fs.path.io]/1: operator<< is "os << quoted(p.string<charT, traits>())"; /3: operator>> reads
// "is >> quoted(tmp); p = tmp;". [fs.path.native.obs], [fs.path.generic.obs]: string(),
// u8string(), u16string(), u32string(), wstring(), generic_string() (POSIX: '/' separators),
// c_str() == native().c_str(), implicit conversion to string_type.
#include <filesystem>
#include <sstream>
#include <string>
#include <type_traits>
#include "check.hpp"

namespace fs = std::filesystem;

static_assert(std::is_same_v<fs::path::value_type, char>);  // POSIX
static_assert(std::is_same_v<fs::path::string_type, std::string>);
static_assert(std::is_convertible_v<fs::path, std::string>);

int main() {
  fs::path p("dir/my file\"x");
  std::ostringstream os;
  os << p;
  CHECK(os.str() == "\"dir/my file\\\"x\"");
  std::istringstream is(os.str() + " rest");
  fs::path q;
  is >> q;
  CHECK(q == p);
  std::string rest;
  is >> rest;
  CHECK(rest == "rest");

  std::istringstream unquoted("plain word");
  fs::path r;
  unquoted >> r;
  CHECK(r.native() == "plain");

  fs::path s("a/b/c.txt");
  CHECK(s.string() == "a/b/c.txt");
  CHECK(s.generic_string() == "a/b/c.txt");
  CHECK(s.u8string() == u8"a/b/c.txt");
  CHECK(s.u16string() == u"a/b/c.txt");
  CHECK(s.u32string() == U"a/b/c.txt");
  CHECK(s.wstring() == L"a/b/c.txt");
  CHECK(std::string(s.c_str()) == "a/b/c.txt");
  CHECK(s.c_str() == s.native().c_str());
  std::string conv = s;
  CHECK(conv == "a/b/c.txt");
  fs::path u(u8"été");  // UTF-8 source
  CHECK(u.u8string() == u8"été");
  fs::path w(L"wide");
  CHECK(w.native() == "wide");
  fs::path from32(U"x/y");
  CHECK(from32.native() == "x/y");

  std::wostringstream wos;
  wos << fs::path("a b");
  CHECK(wos.str() == L"\"a b\"");
  return 0;
}
