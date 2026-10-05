// [allocator.requirements.general]: an allocator need only provide value_type, allocate,
// deallocate, a converting constructor and ==; allocator_traits supplies everything else
// (pointer types, construct/destroy, max_size, propagation traits, rebind). vector and
// basic_string ([container.alloc.reqmts]) work with such an allocator, including in
// constant expressions when its members are constexpr.
// REQUIRES: exceptions
#include <vector>
#include <string>
#include "test_allocators.hpp"
#include "check.hpp"

constexpr bool test() {
  std::vector<int, MinimalAlloc<int>> v;
  for (int i = 0; i < 40; ++i) v.push_back(i);
  v.insert(v.begin(), 3, -1);
  v.erase(v.begin() + 5);
  std::vector<int, MinimalAlloc<int>> w = v;
  w.shrink_to_fit();
  if (w.size() != 42 || w[0] != -1 || w[41] != 39 || !(w == v)) return false;
  if (v.get_allocator() != MinimalAlloc<int>()) return false;
  if (v.max_size() == 0) return false;
  using S = std::basic_string<char, std::char_traits<char>, MinimalAlloc<char>>;
  S s(50, 'x');
  s += "tail";
  S t = s.substr(48);
  if (t != "xxtail") return false;
  std::vector<S, MinimalAlloc<S>> vs;
  vs.emplace_back(t);
  vs.emplace_back(10, 'q');
  if (vs[1].size() != 10 || vs[0] != "xxtail") return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
