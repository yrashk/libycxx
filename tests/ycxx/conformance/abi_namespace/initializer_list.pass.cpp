// The inline ABI namespace std::__y1 (DECISIONS §20.4-20.5): a construct the compilers handle
// specially still works with libycxx's declarations.
// The compiler names std::initializer_list: braced lists deduced by auto, range-for over a braced
// list, and initializer-list constructors.
#include <initializer_list>
#include <vector>
#include <cstdio>
int main() {
  auto il = {1, 2, 3};
  int s = 0;
  for (int i : {4, 5}) s += i;
  std::vector<int> v{6, 7};
  s += static_cast<int>(il.size()) + v[1];
  std::puts(s == 19 ? "ok" : "FAIL");
  return s == 19 ? 0 : 1;
}
