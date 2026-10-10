/* A process whose libycxx allocation table says another libycxx ABI version (DECISIONS §20.6): a C
   program defining and exporting __ycxx_allocation_functions, which every libycxx image then binds
   to, and loading the libycxx plugin argv[1]. The plugin's version check must stop the process at
   load time, naming both versions; prints "loaded" if it did not. */
#include <dlfcn.h>
#include <stdio.h>

struct table { const char* abi; void* entries[20]; };
__attribute__((visibility("default"))) const struct table __ycxx_allocation_functions = {"0.0-skewed", {0}};

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  void* h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!h) { printf("dlopen: %s\n", dlerror()); return 2; }
  printf("loaded\n");
  return 0;
}
