// std::byte may alias any object ([basic.lval]/11.3): the compilers know it by name. If they did
// not recognise it inside an inline namespace, the store through b could be assumed not to
// modify *i and f would return 1 (at -O2).
#include <cstddef>
#include <cstdio>
[[gnu::noinline]] int f(int* i, std::byte* b) { *i = 1; *b = std::byte{2}; return *i; }
int main() {
  int x = 0;
  int r = f(&x, reinterpret_cast<std::byte*>(&x));
  bool ok = r != 1;  // 2 on little-endian targets, 0x02000001 on big-endian ones
  std::puts(ok ? "ok" : "FAIL");
  return ok ? 0 : 1;
}
