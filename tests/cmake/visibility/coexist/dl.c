/* A C host: dlopen()s the two libraries in the order given (RTLD_GLOBAL or RTLD_LOCAL by $DL_MODE),
   calls each check, prints "mine N other N". */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char** argv) {
  const char* mode = getenv("DL_MODE");
  int flags = RTLD_NOW | (mode && strcmp(mode, "local") == 0 ? RTLD_LOCAL : RTLD_GLOBAL);
  int mine = -1, other = -1;
  for (int i = 1; i < argc; ++i) {
    void* h = dlopen(argv[i], flags);
    if (!h) { printf("dlopen %s: %s\n", argv[i], dlerror()); return 2; }
    int (*m)(void) = (int (*)(void))dlsym(h, "mine_check");
    int (*o)(void) = (int (*)(void))dlsym(h, "other_check");
    if (m) mine = m();
    if (o) other = o();
  }
  printf("mine %d other %d\n", mine, other);
  return mine == 31 && other == 31 ? 0 : 1;
}
