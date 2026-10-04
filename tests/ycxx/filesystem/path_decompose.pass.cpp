// [fs.path.decompose]: root_name, root_directory, root_path (= root_name() / root_directory()),
// relative_path, parent_path ("the longest prefix ... that produces one fewer element in its
// iteration"; *this if !has_relative_path()), filename (relative_path().empty() ? path() :
// *--end()), stem, extension, with the draft's examples, on POSIX (where no root-name exists
// for these paths and "/" is the root-directory). Leading "//" is avoided (implementation-
// defined root-name).
#include <filesystem>
#include <string>
#include "check.hpp"

namespace fs = std::filesystem;

struct Row {
  const char* p;
  const char* root_name;
  const char* root_dir;
  const char* root_path;
  const char* rel;
  const char* parent;
  const char* filename;
  const char* stem;
  const char* ext;
};

static const Row rows[] = {
    {"", "", "", "", "", "", "", "", ""},
    {".", "", "", "", ".", "", ".", ".", ""},
    {"..", "", "", "", "..", "", "..", "..", ""},
    {"foo", "", "", "", "foo", "", "foo", "foo", ""},
    {"/", "", "/", "/", "", "/", "", "", ""},
    {"/foo", "", "/", "/", "foo", "/", "foo", "foo", ""},
    {"foo/", "", "", "", "foo/", "foo", "", "", ""},
    {"/foo/", "", "/", "/", "foo/", "/foo", "", "", ""},
    {"foo/bar", "", "", "", "foo/bar", "foo", "bar", "bar", ""},
    {"/foo/bar.txt", "", "/", "/", "foo/bar.txt", "/foo", "bar.txt", "bar", ".txt"},
    {"/foo/bar", "", "/", "/", "foo/bar", "/foo", "bar", "bar", ""},
    {"/foo/bar/", "", "/", "/", "foo/bar/", "/foo/bar", "", "", ""},
    {"/foo/.profile", "", "/", "/", "foo/.profile", "/foo", ".profile", ".profile", ""},
    {".bar", "", "", "", ".bar", "", ".bar", ".bar", ""},
    {"..bar", "", "", "", "..bar", "", "..bar", ".", ".bar"},
    {"foo.bar.baz.tar", "", "", "", "foo.bar.baz.tar", "", "foo.bar.baz.tar", "foo.bar.baz", ".tar"},
    {"a/b/.", "", "", "", "a/b/.", "a/b", ".", ".", ""},
    {"a/b/..", "", "", "", "a/b/..", "a/b", "..", "..", ""},
    {"a.", "", "", "", "a.", "", "a.", "a", "."},
    {"/a///b", "", "/", "/", "a///b", "/a", "b", "b", ""},
    {"a/b.c/d", "", "", "", "a/b.c/d", "a/b.c", "d", "d", ""},
};

int main() {
  for (const Row& r : rows) {
    fs::path p(r.p);
    CHECK(p.root_name().native() == r.root_name);
    CHECK(p.root_directory().native() == r.root_dir);
    CHECK(p.root_path().native() == r.root_path);
    CHECK(p.relative_path().native() == r.rel);
    CHECK(p.parent_path().native() == r.parent);
    CHECK(p.filename().native() == r.filename);
    CHECK(p.stem().native() == r.stem);
    CHECK(p.extension().native() == r.ext);
    // [fs.path.decompose]/10: extension is the suffix not included in stem
    CHECK((p.stem().native() + p.extension().native()) == p.filename().native());
  }
  // Example 2 of [fs.path.decompose]
  fs::path p = "foo.bar.baz.tar";
  std::string exts;
  for (; !p.extension().empty(); p = p.stem()) exts += p.extension().string();
  CHECK(exts == ".tar.baz.bar");
  return 0;
}
