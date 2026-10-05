// [std.modules]/4 (note), /5: headers included after `import std;` redeclare the entities the
// module made reachable; the program sees one entity for each, with both definitions merged.
// MODULES: std
import std;
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "module_check.hpp"

int main() {
  std::vector<std::string> v{"b", "a"};
  std::sort(v.begin(), v.end());
  std::unordered_map<std::string, int> m{{"a", 1}};
  auto p = std::make_shared<int>(4);
  std::flat_set<int> fs{3, 1}; // only through the module
  CHECK(v.front() == "a" && m["a"] == 1 && *p == 4 && *fs.begin() == 1);
  std::cout << "import, then #include: " << v.size() << '\n';
  return 0;
}
