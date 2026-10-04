// [fs.path.append]/2-3: p1 /= p2: if p2.is_absolute() (or has a different root-name) the result
// is p2; otherwise if p2 has a root-directory it replaces the root-directory and relative path;
// otherwise a separator is appended "if has_filename() is true" (or the path is absolute without
// a root-directory), then p2's native pathname. Example 1: path("foo") /= path("") yields
// "foo/"; path("foo") /= path("/bar") yields "/bar". [fs.path.nonmember]: operator/ is
// path(lhs) /= rhs. append(first, last) and append(source).
#include <filesystem>
#include <string>
#include <string_view>
#include "check.hpp"

namespace fs = std::filesystem;

static std::string app(const char* a, const char* b) {
  fs::path p(a);
  p /= fs::path(b);
  return p.native();
}

int main() {
  CHECK(app("foo", "") == "foo/");
  CHECK(app("foo", "/bar") == "/bar");
  CHECK(app("foo", "bar") == "foo/bar");
  CHECK(app("foo/", "bar") == "foo/bar");  // no filename: no separator added
  CHECK(app("", "bar") == "bar");
  CHECK(app("", "") == "");
  CHECK(app("/", "bar") == "/bar");
  CHECK(app("/a", "b/c") == "/a/b/c");
  CHECK(app("a", "b/") == "a/b/");
  CHECK(app("a/b", "/") == "/");
  CHECK(app("a", "..") == "a/..");

  fs::path p("x");
  fs::path& r = (p /= "y");
  CHECK(&r == &p && p == "x/y");
  CHECK((fs::path("a") / "b" / "c").native() == "a/b/c");
  CHECK((fs::path("a") / fs::path("/b")).native() == "/b");
  std::string s = "s";
  CHECK(fs::path("d").append(s).native() == "d/s");
  std::string_view sv = "v";
  CHECK(fs::path("d").append(sv).native() == "d/v");
  const char range[] = "it";
  CHECK(fs::path("d").append(range, range + 2).native() == "d/it");
  CHECK(fs::path("d").append(std::u8string(u8"u")).native() == "d/u");
  return 0;
}
