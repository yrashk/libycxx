// [flat.map.overview]/8, [flat.multimap.overview]: "The program is ill-formed if ... T is not
// the same type as MappedContainer::value_type." Here MappedContainer::value_type is short
// for T int (the same program with vector<int> compiles: flat_map/containers.pass.cpp).
#include <flat_map>
#include <functional>
#include <vector>

int main() {
  std::flat_multimap<int, int, std::less<int>, std::vector<int>, std::vector<short>> m;
  return static_cast<int>(m.size());
}
