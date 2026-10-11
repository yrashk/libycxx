// Both libraries in one process: each must use its own runtime. Prints "mine 31 other 31".
// Built either against libycxx (shared mode) or against the toolchain's library.
#include <cstdio>
extern "C" int other_check();
extern "C" int mine_check();
int main() {
  int m = mine_check(), o = other_check();
  std::printf("mine %d other %d\n", m, o);
  return m == 31 && o == 31 ? 0 : 1;
}
