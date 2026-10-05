// [std.modules]/4 (note): mixing #include and import does not give the same entity two
// attachments. Headers included before `import std;` declare the entities the module re-exports;
// both views are usable together, and the macros of the included headers stay defined.
// MODULES: std
#include <cstdio>
#include <map>
#include <string>
#include <vector>
import std;
#include "module_check.hpp"

int main() {
  std::vector<int> v{1, 2};
  std::map<int, std::string> m{{1, "one"}};
  std::deque<int> d(v.begin(), v.end()); // only through the module
  CHECK(d.size() == 2 && m.at(1) == "one");
  CHECK(std::ranges::equal(v, d));
  std::FILE* f = stdout; // a macro of <cstdio>, included above
  CHECK(f != nullptr && EOF < 0);
  std::println("{}", m);
  return 0;
}
