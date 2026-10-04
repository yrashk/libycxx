// [fs.path.gen]/1-2: lexically_normal() is the normal form of [fs.path.generic] (the eight
// normalization steps), e.g. "foo/./bar/.." -> "foo/" and "foo/.///bar/../" -> "foo/".
// /3-5: lexically_relative with the draft's examples and its empty-path cases; /8:
// lexically_proximate returns *this when lexically_relative is empty.
#include <filesystem>
#include <string>
#include "check.hpp"

namespace fs = std::filesystem;

static std::string norm(const char* s) { return fs::path(s).lexically_normal().native(); }
static std::string rel(const char* a, const char* b) { return fs::path(a).lexically_relative(b).native(); }

int main() {
  CHECK(norm("foo/./bar/..") == "foo/");
  CHECK(norm("foo/.///bar/../") == "foo/");
  CHECK(fs::path("foo/./bar/..").lexically_normal() == "foo/");
  CHECK(norm("") == "");             // step 1
  CHECK(norm(".") == ".");
  CHECK(norm("./") == ".");          // step 4 then step 8
  CHECK(norm("a/..") == ".");        // step 5 then 8
  CHECK(norm("a/../") == ".");
  CHECK(norm("..") == "..");
  CHECK(norm("../") == "..");        // step 7
  CHECK(norm("../..") == "../..");
  CHECK(norm("a/../..") == "..");
  CHECK(norm("/..") == "/");         // step 6
  CHECK(norm("/../a") == "/a");
  CHECK(norm("/a/../../b") == "/b");
  CHECK(norm("a//b///c") == "a/b/c");   // step 3
  CHECK(norm("a/b/../../c/./d/") == "c/d/");
  CHECK(norm("a/./b/.") == "a/b/");
  CHECK(norm("/./") == "/");
  CHECK(norm("x/y/../..") == ".");
  CHECK(norm("x/../y/..") == ".");
  CHECK(norm("../a/../b") == "../b");

  CHECK(rel("/a/d", "/a/b/c") == "../../d");
  CHECK(rel("/a/b/c", "/a/d") == "../b/c");
  CHECK(rel("a/b/c", "a") == "b/c");
  CHECK(rel("a/b/c", "a/b/c/x/y") == "../..");
  CHECK(rel("a/b/c", "a/b/c") == ".");
  CHECK(rel("a/b", "c/d") == "../../a/b");
  CHECK(rel("/a", "b") == "");         // is_absolute differs
  CHECK(rel("a", "/b") == "");
  CHECK(rel("a", "../..") == "");      // n < 0
  CHECK(rel("a", "../b") == "a");      // n == 0: "b" counts +1, ".." counts -1
  CHECK(rel("a/b", "a/b/.") == ".");   // dot is not counted
  CHECK(rel("a/b", "a/b/") == ".");    // empty element is not counted
  CHECK(rel("a/b/", "a/b") == ".");    // 3.7: a->empty()
  CHECK(rel("a/b/c", "a/b/c/..") == "");   // n = -1 (not normalized)
  CHECK(rel("", "") == ".");

  CHECK(fs::path("/a/b").lexically_proximate("/a").native() == "b");
  CHECK(fs::path("/a/b").lexically_proximate("c").native() == "/a/b");
  CHECK(fs::path("a").lexically_proximate("../..").native() == "a");
  return 0;
}
