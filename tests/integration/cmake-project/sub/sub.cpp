#include "sub.hpp"
#include <map>
#include <numeric>
#include <vector>

std::string sub_suffix() { return ""; }
int sub_answer() {
  std::vector<int> v{20, 22};
  std::map<int, int> m{{1, std::accumulate(v.begin(), v.end(), 0)}};
  return m.at(1);
}
