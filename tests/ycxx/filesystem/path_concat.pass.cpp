// [fs.path.concat]/1: operator+= / concat "Appends path(x).native() to the pathname in the
// native format" (no separator logic); /3: a single character. [fs.path.modifiers]:
// remove_filename, replace_filename (remove_filename(); operator/=(replacement);),
// replace_extension (remove the extension, add a dot if the replacement is non-empty and does
// not start with one, then +=), clear, make_preferred (POSIX: no change), swap. The draft's
// examples.
#include <filesystem>
#include <string>
#include "check.hpp"

namespace fs = std::filesystem;

int main() {
  fs::path p("foo");
  p += "bar";
  CHECK(p.native() == "foobar");
  p += fs::path("/x");
  CHECK(p.native() == "foobar/x");
  p += '/';
  CHECK(p.native() == "foobar/x/");
  p += std::string("y");
  p.concat(std::string_view(".z"));
  CHECK(p.native() == "foobar/x/y.z");
  const char s[] = "12";
  p.concat(s, s + 2);
  CHECK(p.native() == "foobar/x/y.z12");
  fs::path e;
  e += "";
  CHECK(e.empty());

  CHECK(fs::path("foo/bar").remove_filename().native() == "foo/");
  CHECK(fs::path("foo/").remove_filename().native() == "foo/");
  CHECK(fs::path("/foo").remove_filename().native() == "/");
  CHECK(fs::path("/").remove_filename().native() == "/");
  CHECK(fs::path("foo").remove_filename().native() == "");
  CHECK(!fs::path("a/b").remove_filename().has_filename());

  CHECK(fs::path("/foo").replace_filename("bar").native() == "/bar");
  CHECK(fs::path("/").replace_filename("bar").native() == "/bar");
  CHECK(fs::path("a/b.c").replace_filename("d").native() == "a/d");
  CHECK(fs::path("a/").replace_filename("d").native() == "a/d");

  CHECK(fs::path("a/b.txt").replace_extension(".md").native() == "a/b.md");
  CHECK(fs::path("a/b.txt").replace_extension("md").native() == "a/b.md");
  CHECK(fs::path("a/b.txt").replace_extension().native() == "a/b");
  CHECK(fs::path("a/b").replace_extension("x").native() == "a/b.x");
  CHECK(fs::path("a/.profile").replace_extension("x").native() == "a/.profile.x");
  CHECK(fs::path("a.tar.gz").replace_extension("").native() == "a.tar");
  CHECK(fs::path("a/").replace_extension("x").native() == "a/.x");
  CHECK(fs::path("x.y").replace_extension(".").native() == "x.");

  fs::path q("foo/bar");
  CHECK(q.make_preferred().native() == "foo/bar");  // POSIX: '/' is the preferred separator
  static_assert(fs::path::preferred_separator == '/');

  fs::path a("a"), b("b/c");
  a.swap(b);
  CHECK(a.native() == "b/c" && b.native() == "a");
  swap(a, b);
  CHECK(a.native() == "a");
  a.clear();
  CHECK(a.empty());
  static_assert(noexcept(a.clear()));
  static_assert(noexcept(a.swap(b)));
  return 0;
}
