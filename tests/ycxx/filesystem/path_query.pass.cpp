// [fs.path.query]: empty() is true iff the generic pathname is empty; has_X() is !X().empty()
// for each decomposition; is_absolute(): on POSIX a path with a root-directory; is_relative()
// is !is_absolute(). [fs.path.construct]: a default path is empty.
#include <filesystem>
#include "check.hpp"

namespace fs = std::filesystem;

static void consistent(const fs::path& p) {
  CHECK(p.has_root_name() == !p.root_name().empty());
  CHECK(p.has_root_directory() == !p.root_directory().empty());
  CHECK(p.has_root_path() == !p.root_path().empty());
  CHECK(p.has_relative_path() == !p.relative_path().empty());
  CHECK(p.has_parent_path() == !p.parent_path().empty());
  CHECK(p.has_filename() == !p.filename().empty());
  CHECK(p.has_stem() == !p.stem().empty());
  CHECK(p.has_extension() == !p.extension().empty());
  CHECK(p.is_relative() == !p.is_absolute());
}

int main() {
  const char* samples[] = {"", "/", "/a", "a", "a/", "a/b.c", "/a/b/", ".", "..", ".x", "a/.."};
  for (const char* s : samples) consistent(fs::path(s));

  CHECK(fs::path().empty());
  CHECK(!fs::path("a").empty());
  CHECK(fs::path("/").is_absolute());
  CHECK(fs::path("/usr/lib").is_absolute());
  CHECK(!fs::path("usr/lib").is_absolute());
  CHECK(fs::path("").is_relative());
  CHECK(!fs::path("/").has_filename());
  CHECK(!fs::path("/").has_relative_path());
  CHECK(fs::path("/").has_parent_path());  // parent_path of "/" is "/" itself
  CHECK(!fs::path("a").has_parent_path());
  CHECK(!fs::path("a/").has_filename());
  CHECK(fs::path("a.b").has_extension());
  CHECK(!fs::path(".b").has_extension());
  static_assert(noexcept(fs::path().empty()));
  return 0;
}
