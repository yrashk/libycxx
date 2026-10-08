/* Loads liba.dylib and libb.dylib (argv[1], argv[2]) and compares the addresses they return. */
#include <dlfcn.h>
#include <stdio.h>
int main(int argc, char** argv) {
  if (argc < 3) return 2;
  void* a = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  void* b = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
  if (!a || !b) { printf("dlopen: %s\n", dlerror()); return 2; }
  int* (*fa)(void) = (int* (*)(void))dlsym(a, "addr_a");
  int* (*fb)(void) = (int* (*)(void))dlsym(b, "addr_b");
  if (!fa || !fb) { printf("dlsym failed\n"); return 2; }
  printf("%s\n", fa() == fb() ? "one object" : "two objects");
  return 0;
}
