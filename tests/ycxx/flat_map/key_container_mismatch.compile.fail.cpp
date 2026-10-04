// [flat.map.overview]/8: "The program is ill-formed if Key is not the same type as
// KeyContainer::value_type or T is not the same type as MappedContainer::value_type."
// Here KeyContainer::value_type is long for Key int (the same program with vector<int>
// compiles: flat_map/types.compile.pass.cpp, flat_map/containers.pass.cpp).
#include <flat_map>
#include <functional>
#include <vector>

int main() {
  std::flat_map<int, int, std::less<int>, std::vector<long>, std::vector<int>> m;
  return static_cast<int>(m.size());
}
