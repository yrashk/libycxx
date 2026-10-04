// [vector.bool.pspc]: every member of vector<bool> and of its reference class is constexpr,
// so vector<bool> works in constant expressions (growth, flip, reference proxies, swap,
// erase_if, comparison).
#include <vector>
#include "check.hpp"

constexpr int count_true(const std::vector<bool>& v) {
  int n = 0;
  for (bool b : v) n += b;
  return n;
}

constexpr bool test() {
  std::vector<bool> v;
  for (int i = 0; i < 300; ++i) v.push_back(i % 2 == 0);
  if (count_true(v) != 150) return false;
  v.flip();
  if (count_true(v) != 150 || v[0]) return false;
  v.insert(v.begin(), 10, true);
  if (count_true(v) != 160) return false;
  std::erase_if(v, [](bool b) { return !b; });
  if (v.size() != 160) return false;
  std::vector<bool> w(3);
  swap(w[0], v[0]);
  if (w[0] != true || v[0] != false) return false;
  w.swap(v);
  if (w.size() != 160 || v.size() != 3) return false;
  return true;
}
static_assert(test());
static_assert(count_true(std::vector<bool>(64, true)) == 64);

int main() {
  CHECK(test());
  return 0;
}
