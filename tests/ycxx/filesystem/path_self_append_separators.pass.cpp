// path operations whose argument is the path itself or a part of it, and leading multiple
// slashes when the implementation does not treat them as a root-name.
// [fs.path.append]/2-3: p /= x appends x (here *this): with no root-name and no root-directory
// in x and has_filename(), a preferred-separator then x.native(); "a/b" /= "a/b" is "a/b/a/b"
// (no precondition excludes x aliasing *this). /5: append(first, last) is
// operator/=(path(first, last)); [fs.path.concat]/1-2: += appends x.native(); concat(x) is
// *this += path(x). [fs.path.generic]/6 normalization: "Replace each directory-separator with a
// preferred-separator" (a directory-separator is one or more slashes), so "//" and "///" -
// when they have no root-name - normalize to "/". [fs.path.decompose]/14: parent_path() is
// "the longest prefix of the generic format pathname of *this that produces one fewer element
// in its iteration": for "//net" (root-directory "//", filename "net") that is "//".
// [fs.path.generic]/4: whether a leading "//" starts a root-name is implementation-defined, so
// those checks only apply when has_root_name() is false.
#include <filesystem>
#include <string>
#include <string_view>
#include "check.hpp"

namespace fs = std::filesystem;

int main() {
  fs::path a = "a/b";
  a /= a;
  CHECK(a.native() == "a/b/a/b");
  a = "a/b/";
  a /= a;
  CHECK(a.native() == "a/b/a/b/");
  a = "/x";
  a /= a;  // x has a root-directory: replaces
  CHECK(a.native() == "/x");
  a = "a/b";
  a += a;
  CHECK(a.native() == "a/ba/b");
  a = "a";
  a.concat(a.native());
  CHECK(a.native() == "aa");
  a = "abc/def";
  a /= std::string_view(a.native()).substr(4);
  CHECK(a.native() == "abc/def/def");
  a = "abc/def";
  a.append(a.native().begin() + 4, a.native().end());
  CHECK(a.native() == "abc/def/def");
  a = "abc/def";
  a.concat(a.native().begin(), a.native().begin() + 3);
  CHECK(a.native() == "abc/defabc");
  a = "abc/def";
  a /= a.filename();
  CHECK(a.native() == "abc/def/def");
  a = "abc/def";
  a = a.parent_path();
  CHECK(a.native() == "abc");
  a = "abc/def";
  a.replace_filename(a.parent_path());
  CHECK(a.native() == "abc/abc");
  a = "x.tar.gz";
  a.replace_extension(a.stem().extension());
  CHECK(a.native() == "x.tar.tar");

  if (!fs::path("//").has_root_name()) {
    CHECK(fs::path("//").lexically_normal().native() == "/");
    CHECK(fs::path("//").root_directory().native() == "/" || fs::path("//").root_directory().native() == "//");
  }
  if (!fs::path("///").has_root_name())
    CHECK(fs::path("///").lexically_normal().native() == "/");
  if (!fs::path("//net").has_root_name()) {
    CHECK(fs::path("//net").parent_path().native() == "//");
    CHECK(fs::path("//net").lexically_normal().native() == "/net");
  }
  if (!fs::path("///a//b").has_root_name()) {
    CHECK(fs::path("///a//b").lexically_normal().native() == "/a/b");
    CHECK(fs::path("///a//b").parent_path().native() == "///a");
  }
  CHECK(fs::path("a//b///").lexically_normal().native() == "a/b/");
  return 0;
}
