// [fs.path.itr]/4: forward traversal: root-name, root-directory, each filename, and "An empty
// element, if a trailing non-root directory-separator is present"; /5 backward traversal is the
// reverse; /2: a bidirectional iterator with value_type path. begin() == end() for an empty
// path.
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>
#include "check.hpp"

namespace fs = std::filesystem;

static std::vector<std::string> fwd(const char* s) {
  std::vector<std::string> v;
  fs::path p(s);
  for (const fs::path& e : p) v.push_back(e.native());
  return v;
}
static std::vector<std::string> bwd(const char* s) {
  std::vector<std::string> v;
  fs::path p(s);
  for (auto it = p.end(); it != p.begin();) v.insert(v.begin(), (--it)->native());
  return v;
}

using V = std::vector<std::string>;

static_assert(std::bidirectional_iterator<fs::path::iterator>);
static_assert(std::is_same_v<std::iter_value_t<fs::path::iterator>, fs::path>);
static_assert(std::is_same_v<fs::path::iterator, fs::path::const_iterator>);

int main() {
  CHECK(fwd("").empty());
  CHECK(fwd("/") == V{"/"});
  CHECK(fwd("/foo/bar/") == (V{"/", "foo", "bar", ""}));
  CHECK(fwd("foo/bar") == (V{"foo", "bar"}));
  CHECK(fwd("foo//bar//") == (V{"foo", "bar", ""}));
  CHECK(fwd("./..") == (V{".", ".."}));
  CHECK(fwd("a/") == (V{"a", ""}));
  for (const char* s : {"", "/", "/foo/bar/", "foo//bar//", "a/b/c", "/a/"}) CHECK(fwd(s) == bwd(s));
  fs::path p("/a/b");
  CHECK(std::distance(p.begin(), p.end()) == 3);
  auto it = p.begin();
  CHECK(*it == "/");
  CHECK(*++it == "a");
  CHECK(*it++ == "a");
  CHECK(it->native() == "b");
  CHECK(++it == p.end());
  CHECK(*--it == "b");
  return 0;
}
