// More POSIX paths through [fs.path.decompose] (no root-name on POSIX for these: none starts
// with exactly two slashes):
//   /4 relative_path: from the first filename after root_path()
//   /5 parent_path: *this if !has_relative_path(), otherwise the longest prefix of the generic
//      pathname producing one fewer element in the iteration ([fs.path.itr]/4: a trailing
//      directory-separator yields an empty final element)
//   /6 filename: relative_path().empty() ? path() : *--end()
//   /8 stem: f if it contains no periods other than a leading period or consists solely of one
//      or two periods, otherwise the prefix of f before its last period; /10 extension: the rest
// and [fs.path.gen]: lexically_normal ([fs.path.generic] steps 1-8), lexically_relative /3
// (3.6: n counts the filename elements of [b, base.end()) that are not dot, dot-dot or empty,
// minus the dot-dot ones; 3.7: n == 0 and (a == end() || a->empty()) gives "."; 3.8: n times
// "..", then each element of [a, end()) appended with /=, so a trailing empty element appends a
// separator, [fs.path.append]/2), lexically_proximate /8-9.
#include <filesystem>
#include <string>
#include "check.hpp"

namespace fs = std::filesystem;

struct Row {
  const char* p;
  const char* root_dir;
  const char* rel;
  const char* parent;
  const char* filename;
  const char* stem;
  const char* ext;
};

static const Row rows[] = {
    {"...", "", "...", "", "...", "..", "."},
    {"....", "", "....", "", "....", "...", "."},
    {".a.", "", ".a.", "", ".a.", ".a", "."},
    {"a..", "", "a..", "", "a..", "a.", "."},
    {"..a.b", "", "..a.b", "", "..a.b", "..a", ".b"},
    {". ", "", ". ", "", ". ", ". ", ""},
    {"/.", "/", ".", "/", ".", ".", ""},
    {"/..", "/", "..", "/", "..", "..", ""},
    {"a//", "", "a//", "a", "", "", ""},
    {"a/./", "", "a/./", "a/.", "", "", ""},
    {"/a/b//", "/", "a/b//", "/a/b", "", "", ""},
    {"a/.b.c", "", "a/.b.c", "a", ".b.c", ".b", ".c"},
    {"a/b/...", "", "a/b/...", "a/b", "...", "..", "."},
    {"a/b/../", "", "a/b/../", "a/b/..", "", "", ""},
    {"/a/./b", "/", "a/./b", "/a/.", "b", "b", ""},
    {"a b/c d.e f", "", "a b/c d.e f", "a b", "c d.e f", "c d", ".e f"},
};

static std::string norm(const char* s) { return fs::path(s).lexically_normal().generic_string(); }
static std::string rel(const char* a, const char* b) { return fs::path(a).lexically_relative(b).generic_string(); }

int main() {
  for (const Row& r : rows) {
    fs::path p(r.p);
    CHECK(p.root_name().empty() && !p.has_root_name());
    CHECK(p.root_directory().native() == r.root_dir);
    CHECK(p.root_path().native() == r.root_dir);
    CHECK(p.relative_path().native() == r.rel);
    CHECK(p.parent_path().native() == r.parent);
    CHECK(p.filename().native() == r.filename);
    CHECK(p.stem().native() == r.stem);
    CHECK(p.extension().native() == r.ext);
    CHECK(p.has_filename() == (*r.filename != 0));
    CHECK(p.has_parent_path() == (*r.parent != 0));
    CHECK(p.has_stem() == (*r.stem != 0) && p.has_extension() == (*r.ext != 0));
    CHECK(p.is_absolute() == (*r.root_dir != 0));
    // [fs.path.decompose]/5: the parent produces one fewer element
    if (p.has_relative_path()) {
      long n = 0, m = 0;
      for (auto it = p.begin(); it != p.end(); ++it) ++n;
      fs::path q = p.parent_path();
      for (auto it = q.begin(); it != q.end(); ++it) ++m;
      CHECK(m == n - 1);
    }
  }

  CHECK(norm("a/b/c/../../../..") == "..");
  CHECK(norm("a/../../..") == "../..");
  CHECK(norm("/../../a/../b") == "/b");
  CHECK(norm("a/./../.") == ".");
  CHECK(norm(".//.") == ".");
  CHECK(norm("a/b/./") == "a/b/");
  CHECK(norm("../a/..") == "..");
  CHECK(norm("a/b/../") == "a/");
  CHECK(norm("a/../b/../") == ".");
  CHECK(norm("./../a") == "../a");
  CHECK(norm("/a/b/../../../c/") == "/c/");
  CHECK(norm("a/.../b") == "a/.../b");  // "..." is an ordinary filename
  CHECK(norm("a/..b/../c") == "a/c");

  CHECK(rel("a/c", "a/b/..") == "c");          // n = 1 - 1 = 0, a -> "c"
  CHECK(rel("a/b/c", "a/x/../y") == "../b/c");  // n = 2 - 1
  CHECK(rel("a/b", "a/b/c/../..") == "");       // n = 1 - 2 < 0
  CHECK(rel("a", "a/./.") == ".");              // dots are not counted
  CHECK(rel("/a/b", "/a/b/c//") == "..");       // the empty element is not counted
  CHECK(rel("a/b/c/", "a/b") == "c/");          // "c", then the empty element appends "/"
  CHECK(rel("a", "/") == "");                   // 3.2 / 3.3
  CHECK(rel("/", "/") == ".");
  CHECK(rel("/a", "/") == "a");
  CHECK(rel("/", "/a") == "..");
  CHECK(rel(".", "a") == "../.");
  CHECK(fs::path("a").lexically_proximate("/b") == "a");
  CHECK(fs::path("/x/y").lexically_proximate("/x/y/z/..") == ".");        // 3.7
  CHECK(fs::path("/x/y").lexically_proximate("/x/y/z/../..") == "/x/y");  // relative is "": *this
  CHECK(fs::path("x/y").lexically_proximate("x") == "y");
  return 0;
}
