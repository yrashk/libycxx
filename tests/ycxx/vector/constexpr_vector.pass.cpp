// [vector.overview]: every member and the non-member functions of vector are constexpr; a
// vector may be created, grown, modified and destroyed during constant evaluation, including
// vectors of class types with non-trivial lifetimes (vector<std::string>, nested vectors).
// [vector.overview]/3: the iterators are constexpr iterators.
#include <vector>
#include <algorithm>
#include <string>
#include <utility>
#include "check.hpp"

constexpr std::vector<int> iota_vec(int n) {
  std::vector<int> v;
  for (int i = 0; i < n; ++i) v.push_back(i);
  return v;
}

constexpr bool test() {
  std::vector<int> v = iota_vec(100);
  v.erase(v.begin() + 10, v.end());
  v.insert(v.begin(), {-2, -1});
  std::reverse(v.begin(), v.end());
  if (v.size() != 12 || v.front() != 9 || v.back() != -2) return false;
  std::erase_if(v, [](int x) { return x < 0; });
  if (v.size() != 10) return false;
  std::vector<std::vector<int>> nested(3, std::vector<int>(2, 7));
  nested[1].push_back(8);
  nested.emplace_back(iota_vec(5));
  if (nested.size() != 4 || nested[1].size() != 3 || nested[3][4] != 4) return false;
  std::vector<std::string> s{"a", "b"};
  s.insert(s.begin(), std::string(50, 'x'));
  s.resize(10, "fill");
  std::vector<std::string> t = s;
  s.clear();
  s.swap(t);
  if (s.size() != 10 || s[0].size() != 50 || s[9] != "fill" || !t.empty()) return false;
  std::vector<int> a{1, 2}, b{1, 3};
  if (!(a < b) || a == b) return false;
  a = std::move(b);
  if (a[1] != 3) return false;
  return true;
}
static_assert(test());
static_assert(iota_vec(50).size() == 50);
static_assert(iota_vec(5)[4] == 4);

int main() {
  CHECK(test());
  return 0;
}
