// [flat.set.overview]/8, [flat.multiset.overview]/8: "The program is ill-formed if Key is not
// the same type as KeyContainer::value_type." Here the container holds long for Key int (with
// vector<int> the program compiles: flat_set/containers.pass.cpp).
#include <flat_set>
#include <functional>
#include <vector>

int main() {
  std::flat_set<int, std::less<int>, std::vector<long>> s;
  return static_cast<int>(s.size());
}
