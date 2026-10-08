// Constant evaluation allows allocation only through std::allocator<T>::allocate/deallocate and
// std::construct_at/destroy_at ([expr.const]), which the compilers recognise by name: constexpr
// std::vector, std::string and std::unique_ptr in a constant expression.
#include <memory>
#include <string>
#include <vector>
#include <cstdio>
consteval int f() {
  std::vector<int> v;
  for (int i = 0; i < 100; ++i) v.push_back(i);
  std::string s(100, 'x');
  s += "tail";
  auto p = std::make_unique<int>(3);
  std::allocator<long> a;
  long* q = a.allocate(2);
  std::construct_at(q, 5L);
  long r = *q;
  std::destroy_at(q);
  a.deallocate(q, 2);
  return v[99] + static_cast<int>(s.size()) + *p + static_cast<int>(r);
}
int main() {
  constexpr int r = f();
  std::puts(r == 99 + 104 + 3 + 5 ? "ok" : "FAIL");
  return r == 211 ? 0 : 1;
}
