// prog (built with libycxx) and host (built with the toolchain's C++ library): each library must
// handle its own exceptions with its own runtime.
#include <cstdio>
extern "C" int other_check();
extern "C" int mine_check();
int main() {
  int m = mine_check(), o = other_check();
  std::printf("mine %d other %d\n", m, o);
  return m == 7 && o == 7 ? 0 : 1;
}
