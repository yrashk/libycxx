// [fs.path.compare]/1: compare() orders by root-name, then root-directory presence, then the
// elements of relative_path() lexicographically (so "a/b" and "a//b" compare equal, and the
// separator sorts before any filename character). [fs.path.nonmember]: operator== is
// compare() == 0, operator<=> is compare() <=> 0 (strong_ordering); hash_value(p) is equal for
// paths that compare equal; hash<path> is enabled.
#include <filesystem>
#include <compare>
#include <functional>
#include <type_traits>
#include "check.hpp"

namespace fs = std::filesystem;

static_assert(std::is_same_v<decltype(fs::path() <=> fs::path()), std::strong_ordering>);

int main() {
  CHECK(fs::path("a/b").compare(fs::path("a//b")) == 0);
  CHECK(fs::path("a/b") == fs::path("a//b"));
  CHECK(fs::path("a/b") != fs::path("a/b/"));
  CHECK(fs::path("a").compare("b") < 0);
  CHECK(fs::path("b").compare(std::string("a")) > 0);
  CHECK(fs::path("a").compare(std::string_view("a")) == 0);
  CHECK(fs::path("a/b").compare("a.b") < 0);      // element-wise: "a" < "a.b"
  CHECK(fs::path("a/b") < fs::path("a-b"));       // "a" < "a-b" although '/' > '-'
  CHECK(fs::path("a") < fs::path("/"));            // no root-directory sorts first
  CHECK(fs::path("/z") > fs::path("a"));
  CHECK(fs::path("") < fs::path("a"));
  CHECK(fs::path("a") < fs::path("a/b"));
  CHECK((fs::path("x") <=> fs::path("x")) == std::strong_ordering::equal);
  CHECK((fs::path("x") <=> fs::path("y")) == std::strong_ordering::less);
  CHECK(fs::path("/a/./b") != fs::path("/a/b"));  // no normalization

  CHECK(fs::hash_value(fs::path("a/b")) == fs::hash_value(fs::path("a//b")));
  CHECK(std::hash<fs::path>()(fs::path("a/b")) == std::hash<fs::path>()(fs::path("a///b")));
  CHECK(std::hash<fs::path>()(fs::path("/x/")) == std::hash<fs::path>()(fs::path("/x//")));
  static_assert(noexcept(fs::path().compare(fs::path())));
  return 0;
}
